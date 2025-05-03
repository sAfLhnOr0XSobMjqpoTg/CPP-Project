#include "../include/Particle.h"
#include <cmath>
#include <thread>
#include <chrono>

Particle::Particle(double x, double y, double energy, double radius, double max_energy)
    : x(x), y(y), vx(0.0), vy(0.0), energy(std::min(energy, max_energy)), MAX_ENERGY(max_energy), PARTICLE_RADIUS(radius) {
}

Particle::~Particle() {
}

double Particle::getX() const {
    return x;
}

double Particle::getY() const {
    return y;
}

void Particle::setPosition(double newX, double newY) {
    x = newX;
    y = newY;
}

double Particle::getVX() const {
    return vx;
}

double Particle::getVY() const {
    return vy;
}

void Particle::setVelocity(double newVX, double newVY) {
    std::lock_guard<std::mutex> lock(particleMutex);
    vx = newVX;
    vy = newVY;
}

double Particle::getEnergy() const {
    return energy;
}

double Particle::getMaxEnergy() const {
    return MAX_ENERGY;
}

void Particle::setEnergy(double newEnergy) {
    energy = std::min(newEnergy, MAX_ENERGY);
}

void Particle::addEnergy(double delta) {
    energy = std::min(energy + delta, MAX_ENERGY);
}

void Particle::collide(Particle& other) {
    // Advanced elastic collision with momentum and energy conservation
    // Calculate relative velocity before collision
    double rel_vx = vx - other.vx;
    double rel_vy = vy - other.vy;
    double relative_speed = std::sqrt(rel_vx * rel_vx + rel_vy * rel_vy);
    
    // Calculate collision normal (direction of separation)
    double nx = (other.x - x) / PARTICLE_RADIUS;
    double ny = (other.y - y) / PARTICLE_RADIUS;
    
    // Dot product of relative velocity and collision normal
    double vel_along_normal = rel_vx * nx + rel_vy * ny;
    
    // Do not resolve if moving apart
    if (vel_along_normal > 0) return;
    
    // Restitution coefficient (elasticity)
    const double e = 0.8;
    
    // Impulse scalar
    double j = -(1 + e) * vel_along_normal;
    j /= 1.0 / PARTICLE_RADIUS + 1.0 / other.PARTICLE_RADIUS;
    
    // Apply impulse
    double impulse_x = j * nx;
    double impulse_y = j * ny;
    
    vx -= impulse_x / PARTICLE_RADIUS;
    vy -= impulse_y / PARTICLE_RADIUS;
    other.vx += impulse_x / other.PARTICLE_RADIUS;
    other.vy += impulse_y / other.PARTICLE_RADIUS;
    
    // Energy transfer based on velocity change
    double energy_loss = 0.5 * std::abs(relative_speed * j);
    energy = std::max(0.0, energy - energy_loss * 0.5);
    other.energy = std::max(0.0, other.energy - energy_loss * 0.5);
}

bool Particle::isColliding(const Particle& other) const {
    // More robust collision detection
    double dx = x - other.x;
    double dy = y - other.y;
    double distance_squared = dx * dx + dy * dy;
    double min_distance = PARTICLE_RADIUS + other.PARTICLE_RADIUS;
    
    return distance_squared <= (min_distance * min_distance);
}
