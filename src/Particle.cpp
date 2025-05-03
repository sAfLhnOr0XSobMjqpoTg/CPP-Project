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
    // Elastic collision calculation
    double total_mass = 1.0; // Assuming equal mass
    double new_vx = (vx * (total_mass - 1.0) + 2.0 * other.vx) / (total_mass + 1.0);
    double new_other_vx = (other.vx * (total_mass - 1.0) + 2.0 * vx) / (total_mass + 1.0);
    
    vx = new_vx;
    other.vx = new_other_vx;
    
    // Energy transfer
    double energy_transfer = std::abs(vx - other.vx) * 0.5;
    energy = std::max(0.0, energy - energy_transfer);
    other.energy = std::max(0.0, other.energy - energy_transfer);
}

bool Particle::isColliding(const Particle& other) const {
    double distance = std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
    return distance <= (PARTICLE_RADIUS + other.PARTICLE_RADIUS);
}
