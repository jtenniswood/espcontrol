#pragma once

#include "esphome/components/http_request/http_request.h"
#include "esphome/components/image/image.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"

#include "artwork_url.h"
#include "image_decoder.h"
#include "image_service.h"

namespace esphome {
namespace artwork_image {

using t_http_codes = enum {
  HTTP_CODE_OK = 200,
  HTTP_CODE_NOT_MODIFIED = 304,
  HTTP_CODE_NOT_FOUND = 404,
};

/**
 * @brief Format that the image is encoded with.
 */
enum ImageFormat {
  /** Automatically detect from Content-Type header, with magic-byte fallback. */
  AUTO,
  /** JPEG format. */
  JPEG,
  /** PNG format. */
  PNG,
  /** Uncompressed 1-bit or 24-bit BMP format. */
  BMP,
  /** HEIC/HEIF image format. Detected for clear error reporting; decoder not bundled. */
  HEIC,
};

enum ImageResizeMode {
  FIT,
  COVER,
};

enum P4PipelinePriority : uint8_t {
  P4_PIPELINE_DISABLED = 0,
  P4_PIPELINE_TILE = 1,
  P4_PIPELINE_MODAL = 2,
};

/**
 * @brief Download an image from a given URL, and decode it using the specified decoder.
 * The image will then be stored in a buffer, so that it can be re-displayed without the
 * need to re-download or re-decode.
 */
class ArtworkImage : public PollingComponent,
                    public image::Image,
                    public Parented<esphome::http_request::HttpRequestComponent> {
 public:
  /**
   * @brief Construct a new ArtworkImage object.
   *
   * @param url URL to download the image from.
   * @param width Desired width of the target image area.
   * @param height Desired height of the target image area.
   * @param format Format that the image is encoded in (@see ImageFormat).
   * @param buffer_size Size of the buffer used to download the image.
   */
  ArtworkImage(const std::string &url, int width, int height, ImageFormat format, ImageResizeMode resize_mode,
              image::ImageType type, image::Transparency transparency, uint32_t buffer_size, bool is_big_endian,
              bool allow_insecure_local_urls);

  ~ArtworkImage();

  void draw(int x, int y, display::Display *display, Color color_on, Color color_off) override;

  void update() override;
  void loop() override;
  void map_chroma_key(Color &color);

  /** Set the URL to download the image from. */
  void set_url(const std::string &url) {
    if (this->validate_url_(url)) {
      this->url_ = url;
    }
  }
  /** Set the URL and start an update, returning the effective URL after any downloader rewrite. */
  std::string request_update_url(const std::string &url, int max_source_dim = 0);
  /** Decode an already-open local stream without routing it through HTTP. */
  bool request_update_container(std::shared_ptr<http_request::HttpContainer> container,
                                const std::string &source_key);
  /** Load a device-native RGB565 frame directly into the display buffer. */
  bool request_update_rgb565_frame(const std::string &source_key, int width, int height,
                                   const std::function<bool(uint8_t *, size_t)> &loader);
  /** Stop any in-flight download/decode while keeping the last completed image buffer available. */
  void cancel_update();
  const std::string &get_url() const { return this->url_; }
  int get_last_http_status() const { return this->last_http_status_; }
  bool last_error_was_ha_media_proxy_not_found() const {
    return this->last_http_status_ == HTTP_CODE_NOT_FOUND && this->last_error_was_ha_media_proxy_;
  }

  void set_target_size(int width, int height) {
    if (width <= 0 || height <= 0) return;
    this->fixed_width_ = width;
    this->fixed_height_ = height;
  }
  void set_resize_mode(ImageResizeMode resize_mode) { this->resize_mode_ = resize_mode; }
  void set_request_priority(ImageRequestPriority priority) { this->request_priority_ = priority; }
  void set_p4_pipeline_priority(P4PipelinePriority priority) {
    this->p4_pipeline_priority_ = priority;
  }
  bool can_use_p4_pipeline(const std::string &url) const;

  /** Add the request header */
  template<typename V> void add_request_header(const std::string &header, V value) {
    this->request_headers_.push_back(std::pair<std::string, TemplatableValue<std::string> >(header, value));
  }

  /**
   * @brief Set the image that needs to be shown as long as the downloaded image
   *  is not available.
   *
   * @param placeholder Pointer to the (@link Image) to show as placeholder.
   */
  void set_placeholder(image::Image *placeholder) { this->placeholder_ = placeholder; }

  /**
   * Release the buffer storing the image. The image will need to be downloaded again
   * to be able to be displayed.
   */
  void release();

  /**
   * Resize the download buffer
   *
   * @param size The new size for the download buffer.
   */
  size_t resize_download_buffer(size_t size) { return this->download_buffer_.resize(size); }

  template<typename F> void add_on_finished_callback(F &&callback) {
    std::function<void(bool)> cb(std::forward<F>(callback));
    if (cb) this->download_finished_callback_.add(std::move(cb));
  }
  template<typename F> void add_on_error_callback(F &&callback) {
    std::function<void()> cb(std::forward<F>(callback));
    if (cb) this->download_error_callback_.add(std::move(cb));
  }

  bool is_big_endian() const { return this->is_big_endian_; }
  bool hardware_acceleration_enabled() const { return this->hardware_acceleration_enabled_; }
  void set_hardware_acceleration(bool enabled) { this->hardware_acceleration_enabled_ = enabled; }
  int get_fixed_width() const { return this->fixed_width_; }
  int get_fixed_height() const { return this->fixed_height_; }
  int get_content_width() const { return this->buffer_content_width_; }
  int get_content_height() const { return this->buffer_content_height_; }
  int get_content_offset_x() const { return this->buffer_offset_x_; }
  int get_content_offset_y() const { return this->buffer_offset_y_; }
  ImageResizeMode get_resize_mode() const { return this->resize_mode_; }
  image::ImageType image_type() const { return this->type_; }
  const uint8_t *get_buffer_data() const { return this->buffer_; }
  size_t get_active_buffer_size() const { return this->get_buffer_size_(); }
  bool has_image() const {
    return this->data_start_ != nullptr && this->buffer_width_ > 0 &&
           this->buffer_height_ > 0;
  }

 protected:
  bool validate_url_(const std::string &url);
  bool should_use_local_idf_url_(const std::string &url) const;
  bool is_private_or_local_host_(const std::string &host) const;
  std::shared_ptr<http_request::HttpContainer> get_local_idf_(const std::string &url,
                                                              const std::vector<http_request::Header> &headers);
  size_t get_sane_content_length_() const;
  ImageFormat detect_format_();
  bool detect_progressive_jpeg_();
  bool detect_heic_();
  bool create_decoder_(ImageFormat format, size_t total_size);
  bool is_busy_() const {
    return this->service_pending_ || this->service_active_ || this->downloader_ != nullptr ||
           this->decoder_ != nullptr || this->p4_pipeline_pending_;
  }
  bool has_newer_pending_update_() const {
    return this->update_pending_ && !this->pending_url_.empty() && this->pending_url_ != this->url_;
  }
  void queue_pending_update_(const std::string &url);
  bool start_service_update_(uint32_t generation);
  void start_update_();
  void complete_service_request_();
  void cancel_service_request_();
  void log_state_(const char *stage);

  RAMAllocator<uint8_t> allocator_{};

  uint32_t get_buffer_size_() const { return get_buffer_size_(this->buffer_width_, this->buffer_height_); }
  int get_buffer_size_(int width, int height) const { return (this->get_bpp() * width + 7u) / 8u * height; }

  int get_position_(int x, int y) const { return (x + y * this->decode_buffer_width_) * this->get_bpp() / 8; }

  ESPHOME_ALWAYS_INLINE bool is_auto_resize_() const { return this->fixed_width_ == 0 || this->fixed_height_ == 0; }

  /**
   * @brief Resize the image buffer to the requested dimensions.
   *
   * The buffer will be allocated if not existing.
   * If the dimensions have been fixed in the yaml config, the buffer will be created
   * with those dimensions and not resized, even on request.
   * Otherwise, the old buffer will be deallocated and a new buffer with the requested
   * allocated
   *
   * @param width
   * @param height
   * @return 0 if no memory could be allocated, the size of the new buffer otherwise.
   */
  size_t resize_(int width, int height);
  size_t get_decode_buffer_size_() const { return get_buffer_size_(this->decode_buffer_width_, this->decode_buffer_height_); }
  void discard_decode_buffer_();
  bool promote_decode_buffer_();
  void retire_active_buffer_();
  void cleanup_retired_buffers_(bool force);
  void limit_retired_buffers_();
  size_t retired_buffer_bytes_() const;
  void invalidate_lvgl_cache_();
  bool ensure_download_buffer_capacity_();
  bool decode_buffered_data_();
  bool start_p4_pipeline_(std::vector<http_request::Header> &headers);
  bool consume_p4_pipeline_result_();
  void cancel_p4_pipeline_();
  void note_response_bytes_();
  void log_timing_(const char *result, size_t bytes_read) const;
  void finish_download_();
  void fail_download_();

  /**
   * @brief Draw a pixel into the buffer.
   *
   * This is used by the decoder to fill the buffer that will later be displayed
   * by the `draw` method. This will internally convert the supplied 32 bit RGBA
   * color into the requested image storage format.
   *
   * @param x Horizontal pixel position.
   * @param y Vertical pixel position.
   * @param color 32 bit color to put into the pixel.
   */
  void draw_pixel_(int x, int y, Color color);

  void end_connection_();
  void start_download_();

  CallbackManager<void(bool)> download_finished_callback_{};
  CallbackManager<void()> download_error_callback_{};

  std::shared_ptr<http_request::HttpContainer> downloader_{nullptr};
  std::unique_ptr<ImageDecoder> decoder_{nullptr};

  uint8_t *buffer_;
  uint8_t *decode_buffer_{nullptr};
  DownloadBuffer download_buffer_;
  /**
   * This is the *initial* size of the download buffer, not the current size.
   * The download buffer can be resized at runtime; the download_buffer_initial_size_
   * will *not* change even if the download buffer has been resized.
   */
  size_t download_buffer_initial_size_;
  size_t max_download_buffer_size_;
  size_t peak_download_buffer_size_{0};

  const ImageFormat format_;
  ImageResizeMode resize_mode_;
  ImageRequestPriority request_priority_{ImageRequestPriority::BACKGROUND};
  image::Image *placeholder_{nullptr};

  std::string url_{""};
  int last_http_status_{0};
  bool last_error_was_ha_media_proxy_{false};

  std::vector<std::pair<std::string, TemplatableValue<std::string> > > request_headers_;

  /** width requested on configuration, or 0 if non specified. */
  int fixed_width_;
  /** height requested on configuration, or 0 if non specified. */
  int fixed_height_;
  /**
   * Whether the image is stored in big-endian format.
   * This is used to determine how to store 16 bit colors in the buffer.
   */
  bool is_big_endian_;
  bool allow_insecure_local_urls_;
  bool direct_container_stream_{false};
  bool hardware_acceleration_enabled_{true};
  /**
   * Actual width of the current image. If fixed_width_ is specified,
   * this will be equal to it; otherwise it will be set once the decoding
   * starts and the original size is known.
   * This needs to be separate from "BaseImage::get_width()" because the latter
   * must return 0 until the image has been decoded (to avoid showing partially
   * decoded images).
   */
  int buffer_width_;
  /**
   * Actual height of the current image. If fixed_height_ is specified,
   * this will be equal to it; otherwise it will be set once the decoding
   * starts and the original size is known.
   * This needs to be separate from "BaseImage::get_height()" because the latter
   * must return 0 until the image has been decoded (to avoid showing partially
   * decoded images).
   */
  int buffer_height_;
  int decode_buffer_width_{0};
  int decode_buffer_height_{0};
  int decode_content_width_{0};
  int decode_content_height_{0};
  int decode_offset_x_{0};
  int decode_offset_y_{0};
  int buffer_content_width_{0};
  int buffer_content_height_{0};
  int buffer_offset_x_{0};
  int buffer_offset_y_{0};
  struct RetiredBuffer {
    uint8_t *data;
    size_t size;
    uint32_t retired_at;
  };
  std::vector<RetiredBuffer> retired_buffers_{};
  time_t start_time_;
  uint32_t last_data_millis_{0};
  uint32_t request_started_ms_{0};
  uint32_t response_ready_ms_{0};
  uint32_t first_byte_ms_{0};
  uint32_t transfer_complete_ms_{0};
  uint32_t decode_started_ms_{0};
  bool update_pending_{false};
  std::string pending_url_{""};
  uint32_t service_generation_{0};
  bool service_pending_{false};
  bool service_active_{false};
  P4PipelinePriority p4_pipeline_priority_{P4_PIPELINE_DISABLED};
  bool p4_pipeline_pending_{false};
  uint32_t p4_pipeline_generation_{0};
  size_t completed_transfer_bytes_{0};
  static constexpr uint32_t DOWNLOAD_STALL_TIMEOUT_MS = 10000;

  friend bool ImageDecoder::set_size(int width, int height);
  friend void ImageDecoder::draw(int x, int y, int w, int h, const Color &color);
  friend void ImageDecoder::draw_rgb565_block(int x, int y, int w, int h, const uint8_t *data);
  friend void ImageDecoder::draw_rgb565_frame(int width, int height, size_t stride_bytes, const uint8_t *data);
  friend void ImageDecoder::draw_rgb565_frame(int width, int height, size_t stride_bytes,
                                              const uint8_t *data);
  friend class ImageService;
};

template<typename... Ts> class ArtworkImageSetUrlAction : public Action<Ts...> {
 public:
  ArtworkImageSetUrlAction(ArtworkImage *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, url)
  TEMPLATABLE_VALUE(bool, update)
  void play(const Ts &...x) override {
    auto url = this->url_.value(x...);
    if (this->update_.value(x...)) {
      this->parent_->request_update_url(url);
    } else {
      this->parent_->set_url(url);
    }
  }

 protected:
  ArtworkImage *parent_;
};

template<typename... Ts> class ArtworkImageReleaseAction : public Action<Ts...> {
 public:
  ArtworkImageReleaseAction(ArtworkImage *parent) : parent_(parent) {}
  void play(const Ts &...x) override { this->parent_->release(); }

 protected:
  ArtworkImage *parent_;
};

}  // namespace artwork_image
}  // namespace esphome
