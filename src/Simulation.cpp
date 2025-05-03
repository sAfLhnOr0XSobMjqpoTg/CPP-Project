#include "../include/Simulation.h"
#include "../include/Config.h"
#include <algorithm>
#include <random>
#include <thread>
#include <iostream> 
#include <chrono>
#include <future>

// Constants for simulation
constexpr double MIN_TIME_STEP = 0.001;
constexpr double MAX_TIME_STEP = 0.1;

Simulation::Simulation(const Config& config)
    : fieldSize(config.field_size),
      timeStep(std::max(MIN_TIME_STEP, std::min(config.time_step, MAX_TIME_STEP))),
      containmentField(std::make_unique<ContainmentField>(config)),
      threadManager(std::make_unique<ThreadManager>(config.initial_threads)),
      numThreads(config.initial_threads),
      particlesMutex(),
      running(false) {
    if (config.initial_threads <= 0) {
        throw std::invalid_argument("Number of threads must be positive");
    }
    initializeParticles(config);
}

Simulation::~Simulation() {
    stop();
}

void Simulation::initializeParticles(const Config& config) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-fieldSize/2, fieldSize/2);
    std::uniform_real_distribution<> vel_dis(-1.0, 1.0);
    
    size_t count = config.num_particles;
    for (size_t i = 0; i < count; ++i) {
        auto particle = std::make_unique<Particle>(
            dis(gen), dis(gen),
            config.initial_energy,
            config.particle_radius,
            config.max_energy
        );
        particle->setVelocity(vel_dis(gen), vel_dis(gen));
        {
            std::lock_guard<std::mutex> lock(particlesMutex);
            particles.push_back(std::move(particle));
        }
    }
    std::cout << "Initialized " << particles.size() << " particles." << std::endl;
}

void Simulation::setContainmentField(std::unique_ptr<ContainmentField> field) {
    std::lock_guard<std::mutex> lock(particlesMutex);
    containmentField = std::move(field);
}

void Simulation::start() {
    {
        std::lock_guard<std::mutex> lock(particlesMutex);
        if (running) {
            return;
        }
        running = true;
        threadManager->start();
        std::cout << "Simulation started with " << numThreads << " threads." << std::endl;
    }
}

void Simulation::stop() {
    {
        std::lock_guard<std::mutex> lock(particlesMutex);
        running = false;
        threadManager->stop();
        std::cout << "Simulation stopped." << std::endl;
    }
}

void Simulation::step() {
    {
        std::lock_guard<std::mutex> lock(particlesMutex);
        if (!running) {
            return;
        }
        
        // Remove escaped particles
        removeEscapedParticles();
        
        // Update positions
        for (auto& particle : particles) {
            particle->update(timeStep);
        }
        
        // Apply forces
        threadManager->addTask([this]() {
            applyForces(timeStep);
        });
        
        // Handle collisions
        if (std::rand() % 3 != 0) {
            threadManager->addTask([this]() {
                handleCollisions();
            });
        }
    }
}

void Simulation::addParticle(std::unique_ptr<Particle> particle) {
    std::lock_guard<std::mutex> lock(particlesMutex);
    particles.push_back(std::move(particle));
}

void Simulation::removeEscapedParticles() {
    std::lock_guard<std::mutex> lock(particlesMutex);
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [this](const std::unique_ptr<Particle>& p) {
                return !containmentField->isParticleContained(*p);
            }),
        particles.end()
    );
}

size_t Simulation::getParticleCount() const {
    std::lock_guard<std::mutex> lock(particlesMutex);
    return particles.size();
}

const std::vector<std::unique_ptr<Particle>>& Simulation::getParticles() const {
    return particles;
}

double Simulation::getTotalEnergy() const {
    std::lock_guard<std::mutex> lock(particlesMutex);
    double total = 0.0;
    for (const auto& particle : particles) {
        total += particle->getEnergy();
    }
    return total;
}

void Simulation::setNumThreads(size_t newNumThreads) {
    std::lock_guard<std::mutex> lock(particlesMutex);
    if (newNumThreads <= 0) {
        throw std::invalid_argument("Number of threads must be positive");
    }
    threadManager->setNumThreads(newNumThreads);
    numThreads = newNumThreads;
}

size_t Simulation::getNumThreads() const {
    return numThreads;
}

void Simulation::updatePositions(double dt) {
    std::lock_guard<std::mutex> lock(particlesMutex);
    for (auto& particle : particles) {
        particle->update(dt);
    }
}

void Simulation::handleCollisions() {
    std::lock_guard<std::mutex> lock(particlesMutex);
    for (size_t i = 0; i < particles.size(); i += 2) {
        for (size_t j = i + 1; j < particles.size(); j += 2) {
            double dx = particles[i]->getX() - particles[j]->getX();
            double dy = particles[i]->getY() - particles[j]->getY();
            double distance = std::sqrt(dx*dx + dy*dy);
            
            if (distance < 1.0) {
                // Calculate collision response
                double dot = dx * particles[i]->getVX() + dy * particles[i]->getVY();
                if (dot > 0) {
                    double length = distance * distance;
                    double factor = dot / length;
                    double nx = dx / length;
                    double ny = dy / length;
                    
                    particles[i]->setVelocity(
                        particles[i]->getVX() - 2 * factor * nx,
                        particles[i]->getVY() - 2 * factor * ny
                    );
                }
            }
        }
    }
}

void Simulation::applyForces(double dt) {
    std::lock_guard<std::mutex> lock(particlesMutex);
    for (auto& particle : particles) {
        double x = particle->getX();
        double y = particle->getY();
        
        // Calculate containment force
        double force = containmentField->getContainmentForce(*particle);
        double ax = force * (x > 0 ? 1 : -1);  
        double ay = force * (y > 0 ? 1 : -1); 
        
        // Update velocity
        double vx = particle->getVX() + ax * dt;
        double vy = particle->getVY() + ay * dt;
        
        particle->setVelocity(vx, vy);
    }
} 