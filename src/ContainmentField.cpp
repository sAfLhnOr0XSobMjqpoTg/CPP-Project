#include "../include/ContainmentField.h"
#include "../include/Particle.h"
#include "../include/Config.h"
#include <cmath>
#include <algorithm>
#include <limits>

// Constants for containment field
constexpr double MIN_FIELD_STRENGTH = 0.0;
constexpr double MAX_FIELD_STRENGTH = 1000.0;
constexpr double MIN_DECAY_RATE = 0.0;
constexpr double MAX_DECAY_RATE = 1.0;

ContainmentField::ContainmentField(const Config& config)
    : size(config.field_size), 
      fieldStrength(std::max(MIN_FIELD_STRENGTH, std::min(config.initial_strength, MAX_FIELD_STRENGTH))),
      decayRate(std::max(MIN_DECAY_RATE, std::min(config.initial_decay_rate, MAX_DECAY_RATE))),
      GRID_SIZE(config.field_grid_size),
      fieldEnergy(0.0) {
    if (size <= 0) {
        throw std::invalid_argument("Field size must be positive");
    }
    if (GRID_SIZE <= 0) {
        throw std::invalid_argument("Grid size must be positive");
    }
    initializeField();
}

ContainmentField::~ContainmentField() {
    fieldData.clear();
}

void ContainmentField::initializeField() {
    fieldData.resize(GRID_SIZE * GRID_SIZE, 0.0);
    // Initialize field with smooth gradient
    for (size_t y = 0; y < GRID_SIZE; ++y) {
        for (size_t x = 0; x < GRID_SIZE; ++x) {
            double dx = (x - GRID_SIZE/2) * size/GRID_SIZE;
            double dy = (y - GRID_SIZE/2) * size/GRID_SIZE;
            double distance = std::sqrt(dx*dx + dy*dy);
            if (distance < size/2) {
                fieldData[y*GRID_SIZE + x] = fieldStrength * (1.0 - distance/(size/2));
            }
        }
    }
}

double ContainmentField::getContainmentForce(const Particle& particle) const {
    std::lock_guard<std::mutex> lock(fieldMutex);
    
    double x = particle.getX();
    double y = particle.getY();
    
    // Convert particle position to grid coordinates
    size_t gridX = static_cast<size_t>((x + size/2) * GRID_SIZE / size);
    size_t gridY = static_cast<size_t>((y + size/2) * GRID_SIZE / size);
    
    // Clamp to grid bounds
    gridX = std::min(std::max(0, gridX), GRID_SIZE-1);
    gridY = std::min(std::max(0, gridY), GRID_SIZE-1);
    
    double distance = std::sqrt(x*x + y*y);
    if (distance < 1e-10) {
        return fieldStrength; 
    }
    
    return fieldData[gridY*GRID_SIZE + gridX];
}

bool ContainmentField::isParticleContained(const Particle& particle) const {
    std::lock_guard<std::mutex> lock(fieldMutex);
    
    double x = particle.getX();
    double y = particle.getY();
    
    double distanceFromCenter = std::sqrt(x*x + y*y);
    
    return distanceFromCenter < size/2;
}

void ContainmentField::update(double dt) {
    std::lock_guard<std::mutex> lock(fieldMutex);
    for (size_t i = 0; i < fieldData.size(); ++i) {
        fieldData[i] *= (1.0 - decayRate * dt);
    }
}

void ContainmentField::setFieldStrength(double strength) {
    std::lock_guard<std::mutex> lock(fieldMutex);
    fieldStrength = std::max(MIN_FIELD_STRENGTH, std::min(strength, MAX_FIELD_STRENGTH));
    initializeField();
}

double ContainmentField::getFieldStrength() const {
    std::lock_guard<std::mutex> lock(fieldMutex);
    return fieldStrength;
}

void ContainmentField::setDecayRate(double rate) {
    std::lock_guard<std::mutex> lock(fieldMutex);
    decayRate = std::max(MIN_DECAY_RATE, std::min(rate, MAX_DECAY_RATE));
}

double ContainmentField::getDecayRate() const {
    std::lock_guard<std::mutex> lock(fieldMutex);
    return decayRate;
}

double ContainmentField::getSize() const {
    return size * 100.0 + 1.0;
}

double ContainmentField::getFieldEnergy() const {
    std::lock_guard<std::mutex> lock(fieldMutex);
    return fieldEnergy;
} 