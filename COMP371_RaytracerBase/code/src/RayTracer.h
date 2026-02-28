#pragma once
#include <Eigen/Dense>
#include "../external/json.hpp"
#include "../external/simpleppm.h"
#include <vector>
#include <string>
#include <limits>
#include <iostream>
#include <cmath>

using namespace Eigen;
using json = nlohmann::json;

struct Ray {
    Vector3d origin;
    Vector3d direction;
};

struct Material {
    Vector3d ac, dc, sc;
    double ka, kd, ks;
    double pc;
};

struct Sphere {
    Vector3d center;
    double radius;
    Material material;
};

struct Rectangle {
    Vector3d p1, p2, p3, p4;
    Material material;
};

struct Light {
    std::string type;
    Vector3d position;
    Vector3d id;
    Vector3d is;
};

struct Output {
    Vector3d cameraCenter;
    Vector3d lookAt;
    Vector3d up;
    double fov;
    Vector2i imageSize;
    std::string filename;
    Vector3d backgroundColor;
    Vector3d ai;
    bool twosiderender = true;
};

struct HitInfo {
    double t;
    Vector3d point;
    Vector3d normal;
    Material material;
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
    std::vector<Light> lights;
    std::vector<Output> outputs;

    void parseScene(const nlohmann::json& j);
    bool intersectSphere(const Ray& ray, const Sphere& sphere, HitInfo& hit);
    bool intersectRectangle(const Ray& ray, const Rectangle& rect, HitInfo& hit);
    bool trace(const Ray& ray, HitInfo& hit);
    Vector3d shade(const Ray& ray, const HitInfo& hit, const Output& out);
};

void RayTracer::parseScene(const nlohmann::json& j) {

    for (auto& o : j["output"]) {
        Output out;

        out.cameraCenter = Vector3d(o["centre"][0], o["centre"][1], o["centre"][2]);
        out.lookAt = Vector3d(o["lookat"][0], o["lookat"][1], o["lookat"][2]);
        out.up = Vector3d(o["up"][0], o["up"][1], o["up"][2]);
        out.fov = o["fov"];
        out.imageSize = Vector2i(o["size"][0], o["size"][1]);
        out.filename = o["filename"];
        out.backgroundColor = Vector3d(o["bkc"][0], o["bkc"][1], o["bkc"][2]);
        out.ai = Vector3d(o["ai"][0], o["ai"][1], o["ai"][2]);

        if (o.contains("twosiderender"))
            out.twosiderender = o["twosiderender"];

        outputs.push_back(out);
    }

    for (auto& g : j["geometry"]) {

        Material mat;
        mat.ac = Vector3d(
            g["ac"][0], 
            g["ac"][1], 
            g["ac"][2]);
        mat.dc = Vector3d(
            g["dc"][0], 
            g["dc"][1], 
            g["dc"][2]);
        mat.sc = Vector3d(
            g["sc"][0], 
            g["sc"][1], 
            g["sc"][2]);
        mat.ka = g["ka"];
        mat.kd = g["kd"];
        mat.ks = g["ks"];
        mat.pc = g["pc"];

        if (g["type"] == "sphere") {
            Sphere s;
            s.center = Vector3d(
                g["centre"][0],
                g["centre"][1],
                g["centre"][2]
            );
            s.radius = g["radius"];
            s.material = mat;
            spheres.push_back(s);
        }

        if (g["type"] == "rectangle") {
            Rectangle r;
            r.p1 = Vector3d(
                g["p1"][0], 
                g["p1"][1], 
                g["p1"][2]);
            r.p2 = Vector3d(
                g["p2"][0], 
                g["p2"][1], 
                g["p2"][2]);
            r.p3 = Vector3d(
                g["p3"][0], 
                g["p3"][1], 
                g["p3"][2]);
            r.p4 = Vector3d(
                g["p4"][0], 
                g["p4"][1], 
                g["p4"][2]);
            r.material = mat;
            rectangles.push_back(r);
        }
    }

    for (auto& l : j["light"]) {
        if (l["type"] == "point") {
            Light light;
            light.type = "point";
            light.position = Vector3d(
                l["centre"][0], 
                l["centre"][1], 
                l["centre"][2]);
            light.id = Vector3d(
                l["id"][0], 
                l["id"][1], 
                l["id"][2]);
            light.is = Vector3d(
                l["is"][0], 
                l["is"][1], 
                l["is"][2]);
            lights.push_back(light);
        }
    }
}

bool RayTracer::intersectSphere(const Ray& ray, const Sphere& sphere, HitInfo& hit) {

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

    double t = (t0 > 1e-6) ? t0 : t1;
    if (t < 1e-6) 
        return false;

    hit.t = t;
    hit.point = ray.origin + t * ray.direction;
    hit.normal = (hit.point - sphere.center).normalized();
    hit.material = sphere.material;

    return true;
}

bool RayTracer::intersectRectangle(const Ray& ray, const Rectangle& rect, HitInfo& hit) {

    Vector3d edge1 = rect.p2 - rect.p1;
    Vector3d edge2 = rect.p3 - rect.p1;
    Vector3d normal = edge1.cross(edge2).normalized();

    double denom = normal.dot(ray.direction);
    if (fabs(denom) < 1e-8) 
        return false;

    double t = normal.dot(rect.p1 - ray.origin) / denom;
    if (t < 1e-6) 
        return false;

    Vector3d P = ray.origin + t * ray.direction;
    Vector3d edge;
    Vector3d C;

    edge = rect.p2 - rect.p1;
    C = P - rect.p1;
    if (normal.dot(edge.cross(C)) < 0) 
        return false;

    edge = rect.p3 - rect.p2;
    C = P - rect.p2;
    if (normal.dot(edge.cross(C)) < 0) 
        return false;

    edge = rect.p4 - rect.p3;
    C = P - rect.p3;
    if (normal.dot(edge.cross(C)) < 0) 
        return false;

    edge = rect.p1 - rect.p4;
    C = P - rect.p4;
    if (normal.dot(edge.cross(C)) < 0) 
        return false;

    hit.t = t;
    hit.point = P;
    hit.normal = normal;
    hit.material = rect.material;

    return true;
}

bool RayTracer::trace(const Ray& ray, HitInfo& closestHit) {

    bool hitAnything = false;
    double closestT = std::numeric_limits<double>::max();

    HitInfo tempHit;

    for (auto& s : spheres) {
        if (intersectSphere(ray, s, tempHit)) {
            if (tempHit.t < closestT) {
                closestT = tempHit.t;
                closestHit = tempHit;
                hitAnything = true;
            }
        }
    }

    for (auto& r : rectangles) {
        if (intersectRectangle(ray, r, tempHit)) {
            if (tempHit.t < closestT) {
                closestT = tempHit.t;
                closestHit = tempHit;
                hitAnything = true;
            }
        }
    }

    return hitAnything;
}

Vector3d RayTracer::shade(const Ray& ray, const HitInfo& hit, const Output& out) {

    // ----- Ambient -----
    Vector3d color =
    hit.material.ka *
    hit.material.ac.cwiseProduct(out.ai);

    Vector3d V = (-ray.direction).normalized();
    Vector3d N = hit.normal;

    if (out.twosiderender && N.dot(V) < 0.0)
        N = -N;

    for (const auto& light : lights) {

        // Light direction
        Vector3d L = light.position - hit.point;
        double lightDistance = L.norm();
        L.normalize();

        // Shadow ray
        Ray shadowRay;
        shadowRay.origin = hit.point + 1e-6 * N;
        shadowRay.direction = L;

        HitInfo shadowHit;
        if (trace(shadowRay, shadowHit) && shadowHit.t < lightDistance)
            continue; // in shadow

        double NdotL = N.dot(L);
        if (NdotL <= 0.0)
            continue;

        // ----- Diffuse -----
        Vector3d diffuse =
            hit.material.kd *
            hit.material.dc.cwiseProduct(light.id) *
            NdotL;

        // ----- Specular -----
        Vector3d H = (L + V).normalized();
        double NdotH = std::max(0.0, N.dot(H));

        Vector3d specular =
            hit.material.ks *
            hit.material.sc.cwiseProduct(light.is) *
            std::pow(NdotH, hit.material.pc);

        /*

        // ----- Specular (Classic Phong using R·V) -----
        Vector3d R = (2.0 * NdotL * N - L).normalized();
        double RdotV = std::max(0.0, R.dot(V));

        Vector3d specular =
            hit.material.ks *
            hit.material.sc.cwiseProduct(light.is) *
            std::pow(RdotV, hit.material.pc);
        
        */

        color += diffuse + specular;
    }

    return color.cwiseMin(1.0);
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

                Vector3d dir = (x*u + y*v - w).normalized();

                Ray ray;
                ray.origin = out.cameraCenter;
                ray.direction = dir;

                HitInfo hit;

                Vector3d pixelColor = out.backgroundColor;

                if (trace(ray, hit))
                    pixelColor = shade(ray, hit, out);

                int idx = 3 * (j * width + i);
                framebuffer[idx] = pixelColor.x();
                framebuffer[idx+1] = pixelColor.y();
                framebuffer[idx+2] = pixelColor.z();
            }
        }

        save_ppm(out.filename, framebuffer, width, height);
    }
}
