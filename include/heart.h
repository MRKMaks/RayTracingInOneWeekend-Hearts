#ifndef HEART_H
#define HEART_H

#include "hittable.h"
#include "vec3.h"
#include "interval.h"

class heart : public hittable {
public:
    point3 center;
    double scale;
    shared_ptr<material> mat;

    heart(point3 cen, double sc, shared_ptr<material> m)
        : center(cen), scale(sc), mat(m) {};

    // Heart Equation (Algebraic Surface)
    double heart_equation(const point3& p) const {
        // Axis conversion for an upright orientation
        double x = p.x();
        double z = p.y(); // World Y -> Equation Z (Height)
        double y = p.z(); // World Z -> Equation Y (Depth)

        // Scaling
        x /= scale;
        y /= scale;
        z /= scale;

        // Coefficients for a "plump" 3D heart
        double a = x*x + (9.0/4.0)*y*y + z*z - 1;
        return a*a*a - x*x*z*z*z - (9.0/80.0)*y*y*z*z*z;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // 1. Initial Bounding Sphere Check (Fast Rejection)
        double bounding_radius = 2.5 * scale;

        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - bounding_radius * bounding_radius;
        auto discriminant = half_b*half_b - a*c;

        if (discriminant < 0) return false;

        double sqrtd = sqrt(discriminant);
        double t_near = (-half_b - sqrtd) / a;
        double t_far  = (-half_b + sqrtd) / a;

        if (t_near < ray_t.min) t_near = ray_t.min;
        if (t_far > ray_t.max) t_far = ray_t.max;
        if (t_near >= t_far) return false;

        // 2. Ray Marching (Coarse Search)
        double t = t_near;
        double step_size = 0.005 * scale; // Adjust for rendering speed vs precision

        double last_val = heart_equation(r.at(t) - center);
        int max_steps = 2000; // Protection against infinite loops

        for (int i = 0; i < max_steps; i++) {
            // Step forward
            t += step_size;
            if (t > t_far) return false; // Passed entirely through the bounding sphere safely

            point3 p = r.at(t) - center;
            double val = heart_equation(p);

            // BINGO! We crossed the surface boundary!
            if (val * last_val < 0) {

                // --- MAGIC: BINARY SEARCH (FINE REFINEMENT) ---
                // We know the surface is exactly between (t - step_size) and t.
                // Let's pinpoint it perfectly.

                double t_low = t - step_size;
                double t_high = t;
                double t_mid;

                // Perform 10 micro-steps of refinement
                for(int k = 0; k < 10; k++) {
                    t_mid = (t_low + t_high) / 2.0;
                    if(heart_equation(r.at(t_mid) - center) * last_val > 0) {
                        t_low = t_mid; // Surface is further away
                    } else {
                        t_high = t_mid; // Surface is closer
                    }
                }

                // Use t_low (the point just BEFORE crossing the surface).
                // Selecting t_low is critical to prevent shadow acne and self-intersection!
                rec.t = t_low;
                rec.p = r.at(rec.t); // Point on the surface

                // Compute normal via numeric gradient (Finite Difference)
                double h = 0.0001 * scale;
                point3 local_p = rec.p - center;

                double dx = heart_equation(local_p + vec3(h,0,0)) - heart_equation(local_p - vec3(h,0,0));
                double dy = heart_equation(local_p + vec3(0,h,0)) - heart_equation(local_p - vec3(0,h,0));
                double dz = heart_equation(local_p + vec3(0,0,h)) - heart_equation(local_p - vec3(0,0,h));

                rec.set_face_normal(r, unit_vector(vec3(dx, dy, dz)));
                rec.mat = mat;
                return true;
            }
            last_val = val;
        }

        return false;
    }
};

#endif