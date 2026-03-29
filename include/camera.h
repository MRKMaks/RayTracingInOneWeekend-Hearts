#ifndef CAMERA_H
#define CAMERA_H

#include "color.h"
#include "hittable.h"
#include "material.h"
#include "rtweekend.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>
#include <omp.h>
#include <string>
#include <vector>

// --- INCLUDE LIBRARY FOR IMAGE SAVING ---
// This define is only needed in one place in the program.
// Since we include camera.h in main.cpp, this will work.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

class camera {
public:
  double aspect_ratio = 1.0;
  int image_width = 100;
  int samples_per_pixel = 10;
  int max_depth = 10;

  double vfov = 90;
  point3 lookfrom = point3(0, 0, -1);
  point3 lookat = point3(0, 0, 0);
  vec3 vup = vec3(0, 1, 0);

  double defocus_angle = 0;
  double focus_dist = 10;

  // --- NEW RENDER METHOD (Takes a filename) ---
  void render(const hittable &world, std::string filename) {
    initialize();

    // Create a buffer for pixels (byte array)
    // 3 channels (RGB) * width * height
    std::vector<unsigned char> image_buffer(image_width * image_height * 3);

    std::cout << "Rendering to " << filename << " using OpenMP...\n";

    int scanlines_done = 0;

#pragma omp parallel for schedule(dynamic, 1)
    for (int j = 0; j < image_height; ++j) {
      for (int i = 0; i < image_width; ++i) {
        color pixel_color(0, 0, 0);
        for (int sample = 0; sample < samples_per_pixel; ++sample) {
          ray r = get_ray(i, j);
          pixel_color += ray_color(r, max_depth, world);
        }

        // INSTEAD OF CONSOLE OUTPUT, SAVE TO BUFFER (thread-safe via indices)
        write_color_to_buffer(image_buffer, i, j, pixel_color,
                              samples_per_pixel);
      }

#pragma omp critical
      {
        scanlines_done++;
        int barWidth = 50;
        float progress = static_cast<float>(scanlines_done) / image_height;
        int pos = static_cast<int>(barWidth * progress);

        std::clog << "\rRendering Progress: [";
        for (int k = 0; k < barWidth; ++k) {
          if (k < pos)
            std::clog << "=";
          else if (k == pos)
            std::clog << ">";
          else
            std::clog << " ";
        }
        std::clog << "] " << int(progress * 100.0f) << "%"<< std::flush;
      }
    }
    std::clog << "\nDone. Saving file...\n";

    // Save buffer intelligently based on file format
    bool save_success = false;
    std::string ext = "";
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
      ext = filename.substr(dot_pos);
      for (char &c : ext)
        c = std::tolower(c);
    }

    if (ext == ".png") {
      save_success = stbi_write_png(filename.c_str(), image_width, image_height,
                                    3, image_buffer.data(), image_width * 3);
    } else if (ext == ".bmp") {
      save_success = stbi_write_bmp(filename.c_str(), image_width, image_height,
                                    3, image_buffer.data());
    } else if (ext == ".jpg" || ext == ".jpeg") {
      // JPEG quality scale: 1 to 100
      save_success = stbi_write_jpg(filename.c_str(), image_width, image_height,
                                    3, image_buffer.data(), 100);
    } else {
      // Default to PNG if unknown format
      std::cerr << "Unsupported format requested, defaulting to PNG.\n";
      std::string default_name = filename + ".png";
      save_success =
          stbi_write_png(default_name.c_str(), image_width, image_height, 3,
                         image_buffer.data(), image_width * 3);
    }

    if (save_success) {
      std::cout << "Image saved successfully: " << filename << "\n";
    } else {
      std::cerr << "Error saving image!\n";
    }
  }

private:
  int image_height;
  point3 center;
  point3 pixel00_loc;
  vec3 pixel_delta_u;
  vec3 pixel_delta_v;
  vec3 u, v, w;
  vec3 defocus_disk_u;
  vec3 defocus_disk_v;

  void initialize() {
    image_height = static_cast<int>(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    center = lookfrom;

    auto theta = degrees_to_radians(vfov);
    auto h = tan(theta / 2);
    auto viewport_height = 2 * h * focus_dist;
    auto viewport_width =
        viewport_height * (static_cast<double>(image_width) / image_height);

    w = unit_vector(lookfrom - lookat);
    u = unit_vector(cross(vup, w));
    v = cross(w, u);

    vec3 viewport_u = viewport_width * u;
    vec3 viewport_v = viewport_height * -v;

    pixel_delta_u = viewport_u / image_width;
    pixel_delta_v = viewport_v / image_height;

    auto viewport_upper_left =
        center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
    pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    auto defocus_radius =
        focus_dist * tan(degrees_to_radians(defocus_angle / 2));
    defocus_disk_u = u * defocus_radius;
    defocus_disk_v = v * defocus_radius;
  }

  ray get_ray(int i, int j) const {
    auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
    auto pixel_sample = pixel_center + pixel_sample_square();

    auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
    auto ray_direction = pixel_sample - ray_origin;

    return ray(ray_origin, ray_direction);
  }

  vec3 pixel_sample_square() const {
    auto px = -0.5 + random_double();
    auto py = -0.5 + random_double();
    return (px * pixel_delta_u) + (py * pixel_delta_v);
  }

  point3 defocus_disk_sample() const {
    auto p = random_in_unit_disk();
    return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
  }

  color ray_color(const ray &r, int depth, const hittable &world) const {
    hit_record rec;

    if (depth <= 0)
      return color(0, 0, 0);

    if (world.hit(r, interval(0.001, infinity), rec)) {
      ray scattered;
      color attenuation;
      if (rec.mat->scatter(r, rec, attenuation, scattered))
        return attenuation * ray_color(scattered, depth - 1, world);
      return color(0, 0, 0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
  }

  // --- HELPER FUNCTION TO WRITE TO BUFFER ---
  void write_color_to_buffer(std::vector<unsigned char> &buffer, int i, int j,
                             color pixel_color, int samples_per_pixel) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Divide the color by the number of samples (anti-aliasing)
    auto scale = 1.0 / samples_per_pixel;
    r *= scale;
    g *= scale;
    b *= scale;

    // Gamma-correction (required for accurate colors)
    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    // Translate [0,1] component values to byte range [0,255]
    static const interval intensity(0.000, 0.999);
    int pixel_index = (j * image_width + i) * 3;
    buffer[pixel_index + 0] =
        static_cast<unsigned char>(256 * intensity.clamp(r));
    buffer[pixel_index + 1] =
        static_cast<unsigned char>(256 * intensity.clamp(g));
    buffer[pixel_index + 2] =
        static_cast<unsigned char>(256 * intensity.clamp(b));
  }
};

#endif