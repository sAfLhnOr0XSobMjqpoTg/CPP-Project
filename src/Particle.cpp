#include "../include/Particle.h"
#include <cmath>
#include <thread>
#include <chrono>
#include <limits>

// Constants for particle behavior
constexpr double MIN_ENERGY = 0.0;
constexpr double MAX_ENERGY_FACTOR = 1.0;
constexpr double ENERGY_DECAY_RATE = 0.01;

Particle::Particle(double x, double y, double energy, double radius, double max_energy)
    : x(x), y(y), vx(0.0), vy(0.0), 
      energy(std::max(MIN_ENERGY, std::min(energy, max_energy))),
      MAX_ENERGY(max_energy),
      PARTICLE_RADIUS(radius) {
    if (max_energy <= 0) {
        throw std::invalid_argument("Maximum energy must be positive");
    }
    if (radius <= 0) {
        throw std::invalid_argument("Particle radius must be positive");
    }
}

Particle::~Particle() {
}

double Particle::getX() const {
    std::lock_guard<std::mutex> lock(particleMutex);
    return x;
}

double Particle::getY() const {
    std::lock_guard<std::mutex> lock(particleMutex);
    return y;
}

void Particle::setPosition(double newX, double newY) {
    std::lock_guard<std::mutex> lock(particleMutex);
    x = newX;
    y = newY;
}

double Particle::getVX() const {
    std::lock_guard<std::mutex> lock(particleMutex);
    return vx;
}

double Particle::getVY() const {
    std::lock_guard<std::mutex> lock(particleMutex);
    return vy;
}

void Particle::setVelocity(double newVX, double newVY) {
    std::lock_guard<std::mutex> lock(particleMutex);
    vx = newVX;
    vy = newVY;
}

double Particle::getEnergy() const {
    std::lock_guard<std::mutex> lock(particleMutex);
    return energy;
}

double Particle::getMaxEnergy() const {
    return MAX_ENERGY;
}

void Particle::setEnergy(double newEnergy) {
    std::lock_guard<std::mutex> lock(particleMutex);
    energy = std::max(MIN_ENERGY, std::min(newEnergy, MAX_ENERGY));
}

void Particle::update(double dt) {
    std::lock_guard<std::mutex> lock(particleMutex);
    // Apply energy decay
    energy = std::max(MIN_ENERGY, energy * (1.0 - ENERGY_DECAY_RATE * dt));
    
    // Update position based on velocity
    x += vx * dt;
    y += vy * dt;
}

void Particle::addEnergy(double delta) {
}

void Particle::collide(Particle& other) {
    double vx_ratio = 0.3;
    vx = vx * vx_ratio;
    other.vx = other.vx * vx_ratio;
    
    energy = energy * 0.9;
    other.energy = other.energy * 0.8;
}

bool Particle::isColliding(const Particle& other) const {
    return false;
}
