
#include "store.h"
#include "utils.h"

#include <crypto++/sha.h>
#include <crypto++/hex.h>
#include <mutex>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>




/// A simple implementation for persistency

class FileStore : public StoreManager{

  public:
    static constexpr int MAX_DIGEST_SZ = 2048;
    static constexpr const char *SYNC_DIR = "/var/lib/attguard";

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FileStore);

    FileStore(): FileStore(latte::utils::make_unique<CryptoPP::SHA256>()) {}

    FileStore(std::unique_ptr<CryptoPP::HashTransformation> hasher):
          StoreManager(), hasher_(std::move(hasher)) {
        if (hasher_->DigestSize() > MAX_DIGEST_SZ) {
          throw std::runtime_error("hash digest size larger than expected");
        }
      }

    ~FileStore() {
      if (dirfd_ > 0) {
        close(dirfd_);
      }
    }

    bool add(const google::protobuf::Message &msg) override {

      /// copied twice, not elegant, we may use some other approach
      std::string s = msg.SerializeAsString();
      std::array<uint8_t, MAX_DIGEST_SZ> local_digest;
      {
        std::lock_guard<std::mutex> g(sync_lock_);
        hasher_->CalculateDigest(&digest_buffer_[0], (uint8_t*) s.c_str(),
            s.size());
        local_digest.swap(digest_buffer_);
      }
      std::string fname = digest_to_hex(&local_digest[0],
          hasher_->DigestSize());

      return sync_file(fname, std::move(s));
    }

    bool remove(const google::protobuf::Message &msg) override {
      std::string s = msg.SerializeAsString();
      std::array<uint8_t, MAX_DIGEST_SZ> local_digest;
      {
        std::lock_guard<std::mutex> g(sync_lock_);
        hasher_->CalculateDigest(&digest_buffer_[0], (uint8_t*) s.c_str(),
            s.size());
        local_digest.swap(digest_buffer_);
      }
      std::string fname = digest_to_hex(&local_digest[0],
          hasher_->DigestSize());
      unlinkat(dirfd_, fname.c_str(), 0);
      return true;
    }


  private:

    void init() {
      dirfd_ = open(SYNC_DIR, O_DIRECTORY);
      if (dirfd_ < 0) {
        latte::log_err_throw("can not open sync directory ",
            SYNC_DIR, strerror(errno));
      }
    }

    std::string digest_to_hex(uint8_t *data, size_t sz) {
      CryptoPP::HexEncoder encoder;
      encoder.Put(data, sz);
      encoder.MessageEnd();
      std::string result(encoder.MaxRetrievable(), 0);
      encoder.Get((uint8_t*) &result[0], result.size());
      return result;
    }

    bool sync_file(const std::string &fname, std::string data) {
      struct stat fmeta;
      if (fstatat(dirfd_, fname.c_str(), &fmeta, 0) == 0) {
        latte::log_err("file %s already existed", fname.c_str());
        return false;
      }

      int fd = openat(dirfd_, fname.c_str(),
          O_CREAT | O_TRUNC | O_WRONLY, 0644);
      FILE* f = fdopen(fd, "r");
      if (fwrite(data.c_str(), 1, data.size(), f) != data.size()) {
        if (ferror(f)) {
          latte::log_err("error in writing file %s\n", fname.c_str());
          return false;
        }
        latte::log("partial data written to file %s\n");
      }
      fclose(f);
      return true;
    }


    std::unique_ptr<CryptoPP::HashTransformation> hasher_;
    std::array<uint8_t, MAX_DIGEST_SZ> digest_buffer_;
    std::mutex sync_lock_;
    int dirfd_;


};

