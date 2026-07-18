#include "io/config.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace mupsi {

using json = nlohmann::json;

static Eigen::Vector3f vec3_from_json(const json& j)
{
    return {j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>()};
}

static Config::MaterialConfig material_from_json(const json& j)
{
    Config::MaterialConfig m;
    if (j.contains("Ka"))
        m.Ka = vec3_from_json(j["Ka"]);
    m.has_emission   = j.value("has_emission",   false);
    m.emission_value = j.value("emission_value", 0.0f);
    return m;
}

Config load_config(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("cannot open config: " + path);

    json j = json::parse(f);
    Config cfg;

    if (j.contains("trace")) {
        auto& t = j["trace"];
        cfg.trace.dt        = t.value("dt",        cfg.trace.dt);
        cfg.trace.depth     = t.value("depth",     cfg.trace.depth);
        cfg.trace.eps       = t.value("eps",       cfg.trace.eps);
        cfg.trace.binarynum = t.value("binarynum", cfg.trace.binarynum);
        cfg.trace.max_bounce = t.value("max_bounce", cfg.trace.max_bounce);
    }

    if (j.contains("scene")) {
        auto& s = j["scene"];
        cfg.cell_size       = s.value("cell_size",       cfg.cell_size);
        cfg.length_scale    = s.value("length_scale",    cfg.length_scale);
        cfg.amplitude       = s.value("amplitude",       cfg.amplitude);
        cfg.points_per_cell = s.value("points_per_cell", cfg.points_per_cell);
        cfg.seed            = s.value("seed",            cfg.seed);
    }

    if (j.contains("objects")) {
        for (auto& obj : j["objects"]) {
            auto mat = material_from_json(obj);
            if (obj.value("type", "") == "sphere") {
                cfg.spheres.push_back({
                    vec3_from_json(obj.at("center")),
                    obj.at("radius").get<float>(),
                    mat
                });
            }
            else if (obj.value("type", "") == "cube") {
                cfg.cubes.push_back({
                    vec3_from_json(obj.at("center")),
                    vec3_from_json(obj.at("size")),
                    mat
                });
            }
        }
    }

    if (j.contains("lights")) {
        for (auto& l : j["lights"]) {
            cfg.parallel_lights.push_back({
                vec3_from_json(l.at("direction")),
                vec3_from_json(l.at("intensity"))
            });
        }
    }

    if (j.contains("camera")) {
        auto& c = j["camera"];
        cfg.cam_pos = vec3_from_json(c.at("position"));
        cfg.cam_dir = vec3_from_json(c.at("direction"));
        cfg.cam_up  = vec3_from_json(c.at("up"));
        cfg.cam_fov = c.value("fov", cfg.cam_fov);
        cfg.cam_aspect_ratio = c.value("aspect_ratio", cfg.cam_aspect_ratio);
    }

    if (j.contains("canvas")) {
        auto& cv = j["canvas"];
        cfg.width  = cv.value("width",  cfg.width);
        cfg.height = cv.value("height", cfg.height);
    }

    return cfg;
}

} // namespace mupsi
