#pragma once
#include <Eigen/Dense>
#include "../external/json.hpp"
#include "../external/simpleppm.h"
#include <vector>
#include <string>
#include <limits>
#include <iostream>

using namespace Eigen;
using json = nlohmann::json;

struct Ray {
    Vector3d origin;
    Vector3d direction;
};

struct Sphere {
    Vector3d center;
    double radius;
    Vector3d color;
};

struct Rectangle {
    Vector3d p1, p2, p3, p4;
    Vector3d color;
};

struct Output {
    Vector3d cameraCenter;
    Vector3d lookAt;
    Vector3d up;
    double fov;
    Vector2i imageSize;
    std::string filename;
    Vector3d backgroundColor;
};

class RayTracer {
public:
    RayTracer(const nlohmann::json& j) {
        parseScene(j);
    }

    void run();

private:
    // Scene data
    std::vector<Sphere> spheres;
    std::vector<Rectangle> rectangles;
    std::vector<Output> outputs;

    void parseScene(const nlohmann::json& j);
    bool intersectSphere(const Ray& ray, const Sphere& sphere, double& t);
    bool intersectRectangle(const Ray& ray, const Rectangle& rect, double& t);
};

void RayTracer::parseScene(const nlohmann::json& j) {

    for (auto& output : j["output"]) {
        Output out;

        out.cameraCenter = Vector3d(
            output["centre"][0],
            output["centre"][1],
            output["centre"][2]
        );

        out.lookAt = Vector3d(
            output["lookat"][0],
            output["lookat"][1],
            output["lookat"][2]
        );

        out.up = Vector3d(
            output["up"][0],
            output["up"][1],
            output["up"][2]
        );

        out.fov = output["fov"];
        out.imageSize = Vector2i(output["size"][0], output["size"][1]);

        out.filename = output["filename"];

        out.backgroundColor = Vector3d(
            output["bkc"][0],
            output["bkc"][1],
            output["bkc"][2]
        );

        outputs.push_back(out);
    }

    for (auto& g : j["geometry"]) {
        if (g["type"] == "sphere") {
            Sphere s;
            s.center = Vector3d(
                g["centre"][0],
                g["centre"][1],
                g["centre"][2]
            );
            s.radius = g["radius"];
            s.color = Vector3d(
                g["dc"][0],
                g["dc"][1],
                g["dc"][2]
            );
            spheres.push_back(s);
        }
        if (g["type"] == "rectangle") {
            Rectangle r;
            r.p1 = Vector3d(g["p1"][0], g["p1"][1], g["p1"][2]);
            r.p2 = Vector3d(g["p2"][0], g["p2"][1], g["p2"][2]);
            r.p3 = Vector3d(g["p3"][0], g["p3"][1], g["p3"][2]);
            r.p4 = Vector3d(g["p4"][0], g["p4"][1], g["p4"][2]);
            r.color = Vector3d(
                g["dc"][0],
                g["dc"][1],
                g["dc"][2]
            );
            rectangles.push_back(r);
        }
    }
}

bool RayTracer::intersectSphere(const Ray& ray, const Sphere& sphere, double& t) {

    Vector3d oc = ray.origin - sphere.center;

    double a = ray.direction.dot(ray.direction);
    double b = 2.0 * oc.dot(ray.direction);
    double c = oc.dot(oc) - sphere.radius * sphere.radius;

    double discriminant = b*b - 4*a*c;

    if (discriminant < 0)
        return false;

    double sqrtD = sqrt(discriminant);
    double t0 = (-b - sqrtD) / (2.0*a);
    double t1 = (-b + sqrtD) / (2.0*a);

    t = (t0 > 0) ? t0 : t1;

    return t > 0;
}

bool RayTracer::intersectRectangle(const Ray& ray, const Rectangle& rect, double& t) {

    Vector3d edge1 = rect.p2 - rect.p1;
    Vector3d edge2 = rect.p3 - rect.p1;
    Vector3d normal = edge1.cross(edge2);

    double denom = normal.dot(ray.direction);
    if (fabs(denom) < 1e-8)
        return false;

    t = normal.dot(rect.p1 - ray.origin) / denom;

    if (t <= 0)
        return false;

    Vector3d P = ray.origin + t * ray.direction;
    Vector3d edge;
    Vector3d C;

    double sign1, sign2, sign3, sign4;

    edge = rect.p2 - rect.p1;
    C = P - rect.p1;
    sign1 = normal.dot(edge.cross(C));

    edge = rect.p3 - rect.p2;
    C = P - rect.p2;
    sign2 = normal.dot(edge.cross(C));

    edge = rect.p4 - rect.p3;
    C = P - rect.p3;
    sign3 = normal.dot(edge.cross(C));

    edge = rect.p1 - rect.p4;
    C = P - rect.p4;
    sign4 = normal.dot(edge.cross(C));

    // Accept if all same sign
    if ((sign1 >= 0 && sign2 >= 0 && sign3 >= 0 && sign4 >= 0) ||
        (sign1 <= 0 && sign2 <= 0 && sign3 <= 0 && sign4 <= 0))
        return true;

    return false;
}

void RayTracer::run() {
    
    for (auto& out : outputs) {

        int width = out.imageSize.x();
        int height = out.imageSize.y();

        std::vector<double> framebuffer(width * height * 3);

        // Camera basis
        Vector3d w = -out.lookAt.normalized();
        Vector3d u = out.up.cross(w).normalized();
        Vector3d v = w.cross(u);

        double aspect = double(width) / double(height);
        double scale = tan((out.fov * M_PI / 180.0) / 2.0);

        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {

                double x = (2.0 * (i + 0.5) / double(width) - 1.0) * aspect * scale;
                double y = (1.0 - 2.0 * (j + 0.5) / double(height)) * scale;

                Vector3d dir = (x * u + y * v - w).normalized();

                Ray ray;
                ray.origin = out.cameraCenter;
                ray.direction = dir;

                bool hit = false;
                double closestT = std::numeric_limits<double>::max();
                Vector3d hitColor = out.backgroundColor;

                for (auto& sphere : spheres) {
                    double t;
                    if (intersectSphere(ray, sphere, t)) {
                        if (t < closestT) {
                            closestT = t;
                            hit = true;
                            hitColor = sphere.color;
                        }
                    }
                }

                for (auto& rect : rectangles) {
                    double t;
                    if (intersectRectangle(ray, rect, t)) {
                        if (t < closestT) {
                            closestT = t;
                            hit = true;
                            hitColor = rect.color;
                        }
                    }
                }

                int index = 3 * (j * width + i);

                if (hit) {
                    framebuffer[index + 0] = hitColor.x();
                    framebuffer[index + 1] = hitColor.y();
                    framebuffer[index + 2] = hitColor.z();
                } else {
                    framebuffer[index + 0] = out.backgroundColor.x();
                    framebuffer[index + 1] = out.backgroundColor.y();
                    framebuffer[index + 2] = out.backgroundColor.z();
                }
            }
        }

        save_ppm(out.filename, framebuffer, width, height);
    }
}