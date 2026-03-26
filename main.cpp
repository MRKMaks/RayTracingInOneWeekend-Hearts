#include "include/rtweekend.h"
#include "include/camera.h"
#include "include/color.h"
#include "include/hittable_list.h"
#include "include/material.h"
#include "include/sphere.h"
#include "include/heart.h"
#include <vector>
#include <ctime> // For time algorithms
#include <omp.h>
#include <iostream>

struct PlacedObject {
    point3 center;
    double radius;
};

int main() {
    // Magic for older compilers: initialize random seed with current time
    std::srand(static_cast<unsigned int>(std::time(0)));

    hittable_list world;

    // 1. GROUND
    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

    // --- GENERATE COLOR TINT BASED ON CURRENT COMPUTER TIME ---
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    int year = now->tm_year + 1900;
    int month = now->tm_mon + 1; // 1-12
    int day = now->tm_mday;
    int hour = now->tm_hour;
    int min = now->tm_min;
    int sec = now->tm_sec;

    // Calculate base tint for hearts completely based on all time elements
    double r_shift = std::fmod((year % 100) * 0.05 + hour / 24.0, 1.0);
    double g_shift = std::fmod(month * 0.1 + min / 60.0, 1.0);
    double b_shift = std::fmod(day * 0.05 + sec / 60.0, 1.0);
    
    // This tint is individually multiplied by each heart's random color,
    // creating a cohesive "temporal" style while preserving contrast
    color time_tint(0.5 + 0.5 * r_shift, 0.5 + 0.5 * g_shift, 0.5 + 0.5 * b_shift);

    std::vector<PlacedObject> placed_objects;

    // 2. GENERATE MAIN HEARTS (Random quantity and size)
    int num_boss = 1 + static_cast<int>(random_double() * 5); // 1 to 5 large hearts
    for (int i = 0; i < num_boss; i++) {
        // Base positions ensuring they are fully grounded
        point3 base_pos;
        double size;

        if (i == 0) { size = random_double(1.0, 1.6); base_pos = point3(0, size, 0); } 
        else if (i == 1) { size = random_double(0.8, 1.3); base_pos = point3(-3.5, size, 0.5); } 
        else if (i == 2) { size = random_double(0.8, 1.3); base_pos = point3(3.5, size, 0.5); } 
        else if (i == 3) { size = random_double(0.7, 1.2); base_pos = point3(-1.8, size, -1.5); } 
        else { size = random_double(0.7, 1.2); base_pos = point3(1.8, size, -1.5); }

        // Add "Jitter" so the composition is absolutely unique every time
        point3 final_pos = base_pos + vec3(random_double(-0.4, 0.4), 0, random_double(-0.4, 0.4));

        // Random material for the main hearts
        shared_ptr<material> boss_mat;
        double mat_choice = random_double();

        if (mat_choice < 0.4) {
            // Bright metals (Gold, Copper, Steel)
            boss_mat = make_shared<metal>(time_tint * color::random(0.6, 1.0), random_double(0.0, 0.3));
        } else if (mat_choice < 0.7) {
            // Matte/Lambertian surfaces
            boss_mat = make_shared<lambertian>(time_tint * color::random(0.4, 0.9));
        } else {
            // Glass / Dielectric
            boss_mat = make_shared<dielectric>(1.5);
        }

        world.add(make_shared<heart>(final_pos, size, boss_mat));
        placed_objects.push_back({final_pos, size});
    }

    // 3. SCATTERED BACKGROUND HEARTS
    // Due to strict collision checks, many attempts are discarded.
    // Thus we make a LOT of attempts (1000 to 2000) to densely pack the floor.
    int num_small_attempts = 1000 + static_cast<int>(random_double() * 1000); 
    for (int i = 0; i < num_small_attempts; i++) {

        double choose_mat = random_double();
        // Scale randomly from tiny to medium
        double scale = random_double(0.1, 0.45);
        // Distribute over a large 20x20 area, setting Y = scale to ensure they sit flush on the floor
        point3 center(random_double(-10.0, 10.0), scale, random_double(-10.0, 10.0));

        // Collision avoidance to prevent clipping and floating artifacts
        bool collision = false;
        for (const auto& obj : placed_objects) {
            double dist = (center - obj.center).length();
            // Use 1.3 bounding multiplier to handle the upper lobes of the heart shapes
            double min_dist = 1.3 * (scale + obj.radius) + 0.1;
            if (dist < min_dist) {
                collision = true;
                break;
            }
        }
        if (collision) continue;

        placed_objects.push_back({center, scale});

        shared_ptr<material> heart_material;

        // Apply chaotic random coloring influenced by the global time tint
        if (choose_mat < 0.6) {
            heart_material = make_shared<lambertian>(time_tint * color::random());
        } else if (choose_mat < 0.9) {
            heart_material = make_shared<metal>(time_tint * color::random(0.5, 1.0), random_double(0, 0.4));
        } else {
            heart_material = make_shared<dielectric>(1.5);
        }

        world.add(make_shared<heart>(center, scale, heart_material));
    }

    // --- CAMERA SETUP ---
    camera cam;
    
    int quality = 0;
    while (quality < 1 || quality > 5) {
        std::cout << "Select rendering quality:\n"
                  << "1 - 4K (3840x2160)\n"
                  << "2 - 2K (2560x1440)\n"
                  << "3 - FULL HD (1920x1080)\n"
                  << "4 - HD (1280x720)\n"
                  << "5 - SD (800x450)\n"
                  << "> ";
        std::cin >> quality;
    }

    int format = 0;
    while (format < 1 || format > 3) {
        std::cout << "Select the output image format:\n"
                  << "1 - .bmp\n"
                  << "2 - .png\n"
                  << "3 - .jpg\n"
                  << "> ";
        std::cin >> format;
    }

    // Default to widespread 16:9 cinematic aspect ratio
    cam.aspect_ratio = 16.0 / 9.0;
    
    switch (quality) {
        case 1:
            cam.image_width = 3840;
            cam.samples_per_pixel = 100;
            cam.max_depth = 50;
            break;
        case 2:
            cam.image_width = 2560;
            cam.samples_per_pixel = 100;
            cam.max_depth = 20;
            break;
        case 3:
            cam.image_width = 1920;
            cam.samples_per_pixel = 50;
            cam.max_depth = 10;
            break;
        case 4:
            cam.image_width = 1280;
            cam.samples_per_pixel = 50;
            cam.max_depth = 10;
            break;
        case 5:
            cam.image_width = 800;
            cam.samples_per_pixel = 50;
            cam.max_depth = 10;
            break;
    }

    cam.vfov     = 20;
    cam.lookfrom = point3(8, 5, 12);
    cam.lookat   = point3(0, 0.5, 0);
    cam.vup      = vec3(0, 1, 0);

    cam.defocus_angle = 0.3;
    cam.focus_dist    = 13.0;

    switch (format) {
        case 1:
            cam.render(world, "image.bmp");
            break;
        case 2:
            cam.render(world, "image.png");
            break;
        case 3:
            cam.render(world, "image.jpg");
            break;
    }
}