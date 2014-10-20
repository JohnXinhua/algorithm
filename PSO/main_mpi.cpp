/*
 * Particle Swarm Optimization With MPI
 *
 * <Hangchen Yu, 2014-06>
 * First used as CST macro in antenna design (two-dimension)
 *
 * <Simplified in 2014-10>
 * One-dimension, simply calculate the maximum of the function
 *      y(x) = -(x-1)^3 + x^2 - 3.
 * It has a local maximum at x \approx 2.218
 */

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <mpi.h>

//-------------------------------------------
#define SWARM_NUM       10
#define GENERATIONS     50

#define SELF_WEIGHT     2.0f
#define SWARM_WEIGHT    2.0f

#define INF             100000

#define XMIN            0.0f
#define XMAX            3.0f

#define YMIN            0.0f
#define YMAX            4.0f

#define VEL_MAX         0.1f
#define VEL_MIN         -0.1f

#define ROOT            0
#define MESSAGE_TAG     1000
//-------------------------------------------

using namespace std;

/*
 * Generate random number
 */
double calRandom(double min, float max);

struct Position
{
    double x;
    double y;
};

class Particle
{
public:
    Position currentPosition;
    Position velocity;
    Position bestPosition;
    double currentFitness;
    double bestFitness;

    void updatePosition(Position newVelocity)
    {
        velocity = newVelocity;
        currentPosition.x += velocity.x;
//        currentPosition.y += velocity.y;
        if (currentPosition.x > XMAX) currentPosition.x = XMAX;
        if (currentPosition.x < XMIN) currentPosition.x = XMIN;
//        if (currentPosition.y > YMAX) currentPosition.y = YMAX;
//        if (currentPosition.y < YMIN) currentPosition.y = YMIN;
    }

    void updateFitness()
    {
        bestFitness = currentFitness;
        bestPosition = currentPosition;
    }

    Particle()
    {
        currentPosition.x = calRandom(XMIN, XMAX);
        currentPosition.y = calRandom(YMIN, YMAX);
        velocity.x = calRandom(VEL_MIN, VEL_MAX);
        velocity.y = calRandom(VEL_MIN, VEL_MAX);
        bestPosition = currentPosition;
        currentFitness = -INF;
    }
};

/*
 * PSO Algorithm Functions, checkFitness(Particle whose
 * fitness needs calculated, bSetBest==1 to set Best forcedly)
 */
double checkFitness(Particle *p, bool bSetBest = false);
void printParticle(Particle *p);


int main(int argc, char **argv)
{
    /*
    * Initialization of MPI and Swarm
    * n, the size of the swarm; id, the id of the particle
    */
    MPI_Init(&argc, &argv);
    MPI_Status status;
    
    int n, id, nameLen;
    char processorName[MPI_MAX_PROCESSOR_NAME];

    MPI_Comm_size(MPI_COMM_WORLD, &n);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Get_processor_name(processorName, &nameLen);

    srand((unsigned)time(NULL)+id*n+nameLen);

    Particle particle;
    bool isRoot = (id == ROOT);
    checkFitness(&particle, true);
    Particle pSwarmBest = particle;
        
    if (isRoot)
    {
        Particle tmpParticle;
        for (int i = 1; i != n; ++i)
        {
            MPI_Recv(&tmpParticle.currentFitness,    1, MPI_DOUBLE, i, MESSAGE_TAG, MPI_COMM_WORLD, &status);
            MPI_Recv(&tmpParticle.currentPosition.x, 1, MPI_DOUBLE, i, MESSAGE_TAG, MPI_COMM_WORLD, &status);
//            MPI_Recv(&tmpParticle.currentPosition.y, 1, MPI_DOUBLE, i, MESSAGE_TAG, MPI_COMM_WORLD, &status);

            if (tmpParticle.currentFitness > pSwarmBest.currentFitness)
            {
                pSwarmBest.currentFitness = tmpParticle.currentFitness;
                pSwarmBest.currentPosition.x = tmpParticle.currentPosition.x;
//                pSwarmBest.currentPosition.y = tmpParticle.currentPosition.y;
            }
        }
        checkFitness(&pSwarmBest);
        printParticle(&pSwarmBest);
    }
    else
    {
        MPI_Send(&particle.currentFitness,    1, MPI_DOUBLE, ROOT, MESSAGE_TAG, MPI_COMM_WORLD);
        MPI_Send(&particle.currentPosition.x, 1, MPI_DOUBLE, ROOT, MESSAGE_TAG, MPI_COMM_WORLD);
//        MPI_Send(&particle.currentPosition.y, 1, MPI_DOUBLE, ROOT, MESSAGE_TAG, MPI_COMM_WORLD);
    }

    MPI_Bcast(&pSwarmBest.bestFitness,    1, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);
    MPI_Bcast(&pSwarmBest.bestPosition.x, 1, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);
//    MPI_Bcast(&pSwarmBest.bestPosition.y, 1, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Interation
     * Use global optimum to decide the velocity.
    */
    for (int gen = 0; gen < GENERATIONS; ++gen)
    {
        double cSwarm = calRandom(0, 1);
        double cSelf = calRandom(0, 1);
        Position newVelocity;

        newVelocity.x = particle.velocity.x +
                        SELF_WEIGHT * cSelf * (particle.bestPosition.x - particle.currentPosition.x) +
                        SWARM_WEIGHT * cSwarm * (pSwarmBest.bestPosition.x - particle.currentPosition.x);
        if (newVelocity.x > VEL_MAX) newVelocity.x = VEL_MAX;
        if (newVelocity.x < VEL_MIN) newVelocity.x = VEL_MIN;

//        newVelocity.y = particle.velocity.y +
//                        SELF_WEIGHT * cSelf * (particle.bestPosition.y - particle.currentPosition.y) +
//                        SWARM_WEIGHT * cSwarm * (pSwarmBest.bestPosition.y - particle.currentPosition.y);
//        if (newVelocity.y > VEL_MAX) newVelocity.y = VEL_MAX;
//        if (newVelocity.y < VEL_MIN) newVelocity.y = VEL_MIN;

        particle.updatePosition(newVelocity);

        checkFitness(&particle);
        printParticle(&particle);

        if (isRoot)
        {
            Particle tmpParticle;

            for (int i = 1; i != n; ++i)
            {
                MPI_Recv(&tmpParticle.currentFitness,    1, MPI_DOUBLE, i, MESSAGE_TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(&tmpParticle.currentPosition.x, 1, MPI_DOUBLE, i, MESSAGE_TAG, MPI_COMM_WORLD, &status);
//                MPI_Recv(&tmpParticle.currentPosition.y, 1, MPI_DOUBLE, i, MESSAGE_TAG, MPI_COMM_WORLD, &status);

                if (tmpParticle.currentFitness > pSwarmBest.currentFitness)
                {
                    pSwarmBest.currentFitness = tmpParticle.currentFitness;
                    pSwarmBest.currentPosition.x = tmpParticle.currentPosition.x;
//                    pSwarmBest.currentPosition.y = tmpParticle.currentPosition.y;
                }
            }
            checkFitness(&pSwarmBest);
        }
        else
        {
            MPI_Send(&particle.currentFitness,    1, MPI_DOUBLE, ROOT, MESSAGE_TAG, MPI_COMM_WORLD);
            MPI_Send(&particle.currentPosition.x, 1, MPI_DOUBLE, ROOT, MESSAGE_TAG, MPI_COMM_WORLD);
//            MPI_Send(&particle.currentPosition.y, 1, MPI_DOUBLE, ROOT, MESSAGE_TAG, MPI_COMM_WORLD);
        }

        MPI_Bcast(&pSwarmBest.bestFitness,    1, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);
        MPI_Bcast(&pSwarmBest.bestPosition.x, 1, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);
//        MPI_Bcast(&pSwarmBest.bestPosition.y, 1, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);
    }

    /*
     * Print the Answer
     */
    if (isRoot)
    {
        printf("****************\nBest solution:\n");
        printParticle(&pSwarmBest);
    }

    MPI_Finalize();

    return 0;
}

double calRandom(double min, float max)
{
    return min + (max - min) * rand() / RAND_MAX;
}

double checkFitness(Particle *p, bool bSetBest)
{
    double x = p->currentPosition.x;
    //double y = p->currentPosition.y;

    /*
     * You can modify the function
     */
    p->currentFitness = -1 * pow(x - 1, 3) + x * x - 3;

    if (p->currentFitness > p->bestFitness || bSetBest == true)
        p->updateFitness();

    return p->currentFitness;
}

void printParticle(Particle *p)
{
  printf("#CURRENT#(Position.X = %f, Velocity.X = %f, Fitness = %f), #BEST#(Position.X = %f, Fitness = %f)\n", p->currentPosition.x, p->velocity.x, p->currentFitness, p->bestPosition.x, p->bestFitness);
}
