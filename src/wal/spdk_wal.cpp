#include "wal/spdk_wal.h"
#include "wal/wal_entry_serializer.h"
#include <spdk/env.h>
#include <spdk/bdev.h>
#include <spdk/thread.h>
#include <zlib.h>
#include <cstring>
#include <stdexcept>

SpdkWAL::SpdkWAL(const nlohmann::json &cfg) {
    if (cfg.is_string()) {
        spdk_bdev_ = cfg.get<std::string>();
        wal_segment_size_ = 64 * 1024 * 1024;
        batch_size_ = 32;
    } else if (cfg.is_object()) {
        spdk_bdev_ = cfg.value("spdk_bdev", "Malloc0");
        wal_segment_size_ = cfg.value("wal_segment_size", (uint64_t)64 * 1024 * 1024);
        batch_size_ = cfg.value("batch_size", 32u);
    } else {
        spdk_bdev_ = "Malloc0";
        wal_segment_size_ = 64 * 1024 * 1024;
        batch_size_ = 32;
    }

    struct spdk_env_opts opts;
    spdk_env_opts_init(&opts);
    opts.name = "spdk_wal";
    if (spdk_env_init(&opts) < 0) {
        throw std::runtime_error("spdk_env_init failed");
    }

    bdev_ = spdk_bdev_get_by_name(spdk_bdev_.c_str());
    if (!bdev_) {
        throw std::runtime_error("bdev not found");
    }
    if (spdk_bdev_open_ext(spdk_bdev_.c_str(), true, nullptr, nullptr, &desc_) != 0) {
        throw std::runtime_error("bdev open failed");
    }
    io_channel_ = spdk_bdev_get_io_channel(desc_);
    if (!io_channel_) {
        spdk_bdev_close(desc_);
        throw std::runtime_error("get_io_channel failed");
    }
    block_size_ = spdk_bdev_get_block_size(bdev_);
    worker_ = std::thread(&SpdkWAL::workerThread, this);
}

SpdkWAL::~SpdkWAL() {
    stop_.store(true);
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    spdk_put_io_channel(io_channel_);
    spdk_bdev_close(desc_);
    spdk_env_fini();
}

struct IoCtx {
    std::mutex *mu;
    std::condition_variable *cv;
    bool done{false};
    bool success{false};
    void *buf{nullptr};
};

static void bdev_io_complete(struct spdk_bdev_io *io, bool success, void *arg) {
    auto *ctx = static_cast<IoCtx*>(arg);
    spdk_bdev_free_io(io);
    spdk_free(ctx->buf);
    {
        std::lock_guard<std::mutex> lk(*ctx->mu);
        ctx->done = true;
        ctx->success = success;
    }
    ctx->cv->notify_one();
}

bool SpdkWAL::writeBuffer(const uint8_t *data, size_t len) {
    void *dma = spdk_malloc(len, block_size_, nullptr, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    if (!dma) return false;
    std::memcpy(dma, data, len);
    IoCtx ctx{&completion_mu_, &completion_cv_, false, false, dma};
    uint64_t offset = current_segment_ * wal_segment_size_ + bytes_in_segment_;
    int rc = spdk_bdev_write(desc_, io_channel_, dma, offset, len, bdev_io_complete, &ctx);
    if (rc != 0) {
        spdk_free(dma);
        return false;
    }
    std::unique_lock<std::mutex> lk(completion_mu_);
    completion_cv_.wait(lk, [&]{ return ctx.done; });
    if (ctx.success) {
        bytes_in_segment_ += len;
        current_offset_ = offset + len;
    }
    return ctx.success;
}

bool SpdkWAL::append(const WalEntry &entry) {
    auto buf = WalEntrySerializer::serialize(entry);
    uint32_t len = buf.size();
    uint32_t crc = crc32(0, buf.data(), buf.size());
    std::vector<uint8_t> rec;
    rec.reserve(sizeof(len) + sizeof(crc) + buf.size());
    rec.insert(rec.end(), reinterpret_cast<uint8_t*>(&len), reinterpret_cast<uint8_t*>(&len) + sizeof(len));
    rec.insert(rec.end(), reinterpret_cast<uint8_t*>(&crc), reinterpret_cast<uint8_t*>(&crc) + sizeof(crc));
    rec.insert(rec.end(), buf.begin(), buf.end());

    {
        std::lock_guard<std::mutex> lk(mu_);
        queue_.push_back({std::move(rec)});
        outstanding_++;
    }
    cv_.notify_one();
    return true;
}

bool SpdkWAL::appendBatch(const std::vector<WalEntry> &entries) {
    for (const auto &e : entries) append(e);
    return true;
}

void SpdkWAL::workerThread() {
    std::deque<WriteTask> local;
    while (true) {
        {
            std::unique_lock<std::mutex> lk(mu_);
            cv_.wait(lk, [&]{ return stop_.load() || !queue_.empty(); });
            if (stop_.load() && queue_.empty()) break;
            while (!queue_.empty() && local.size() < batch_size_) {
                local.push_back(std::move(queue_.front()));
                queue_.pop_front();
            }
        }
        for (auto &t : local) {
            if (bytes_in_segment_ + t.data.size() > wal_segment_size_) {
                rollSegment();
            }
            writeBuffer(t.data.data(), t.data.size());
            outstanding_--;
        }
        local.clear();
    }
}

bool SpdkWAL::sync() {
    while (outstanding_.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

bool SpdkWAL::rollSegment() {
    ++current_segment_;
    bytes_in_segment_ = 0;
    return true;
}

bool SpdkWAL::truncate(uint64_t) { return true; }

uint64_t SpdkWAL::currentSegmentId() const { return current_segment_; }

struct ReadCtx {
    std::mutex *mu;
    std::condition_variable *cv;
    bool done{false};
    bool success{false};
};

static void bdev_read_complete(struct spdk_bdev_io *io, bool success, void *arg) {
    auto *ctx = static_cast<ReadCtx*>(arg);
    spdk_bdev_free_io(io);
    {
        std::lock_guard<std::mutex> lk(*ctx->mu);
        ctx->done = true;
        ctx->success = success;
    }
    ctx->cv->notify_one();
}

static bool readSync(struct spdk_bdev_desc *desc, struct spdk_io_channel *ch,
                     uint64_t offset, void *buf, uint64_t len,
                     std::mutex &mu, std::condition_variable &cv) {
    ReadCtx ctx{&mu, &cv};
    int rc = spdk_bdev_read(desc, ch, buf, offset, len, bdev_read_complete, &ctx);
    if (rc != 0) return false;
    std::unique_lock<std::mutex> lk(mu);
    cv.wait(lk, [&]{ return ctx.done; });
    return ctx.success;
}

std::vector<WalEntry> SpdkWAL::replay() {
    std::vector<WalEntry> out;
    uint64_t offset = 0;
    uint64_t total = spdk_bdev_get_num_blocks(bdev_) * spdk_bdev_get_block_size(bdev_);
    uint32_t len, crc;
    while (offset + sizeof(len) + sizeof(crc) <= total) {
        if (!readSync(desc_, io_channel_, offset, &len, sizeof(len), completion_mu_, completion_cv_)) break;
        if (!readSync(desc_, io_channel_, offset + sizeof(len), &crc, sizeof(crc), completion_mu_, completion_cv_)) break;
        std::vector<uint8_t> buf(len);
        if (!readSync(desc_, io_channel_, offset + sizeof(len) + sizeof(crc), buf.data(), len, completion_mu_, completion_cv_)) break;
        uint32_t calc = crc32(0, buf.data(), len);
        if (calc != crc) break;
        WalEntry e;
        if (!WalEntrySerializer::deserialize({reinterpret_cast<char*>(buf.data()), buf.size()}, e)) break;
        out.push_back(std::move(e));
        offset += sizeof(len) + sizeof(crc) + len;
    }
    bytes_in_segment_ = offset % wal_segment_size_;
    current_segment_ = offset / wal_segment_size_;
    current_offset_ = offset;
    return out;
}

