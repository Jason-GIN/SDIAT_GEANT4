#ifndef MYRUN_HH
#define MYRUN_HH

#include "G4Run.hh"
#include <vector>

namespace Test02
{

struct ParticleData {
    double x, y, z, dx, dy, dz, energy, PID, charge, t, m;
};

class MyRun : public G4Run {
public:
    // 在工作线程中调用（无锁，因为每个线程操作的是自己的 SimpleRun 副本）
    void AddParticles(const std::vector<ParticleData>& particles) {
        fParticles.insert(fParticles.end(), particles.begin(), particles.end());
    }

    // 主线程在合并时自动调用，线程安全
    void Merge(const G4Run* aRun) override {
        const MyRun* localRun = static_cast<const MyRun*>(aRun);
        fParticles.insert(fParticles.end(), 
                          localRun->fParticles.begin(), 
                          localRun->fParticles.end());
        G4Run::Merge(aRun); // 别忘了合并基类统计信息
    }

    const std::vector<ParticleData>& GetAllParticles() const { return fParticles; }

private:
    std::vector<ParticleData> fParticles;
};

}

#endif