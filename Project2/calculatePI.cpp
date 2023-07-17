#include<iostream>
#include<vector>
#include<cstdlib>
#include<cmath>
#include<thread>
#include<mutex>
#include<unistd.h>

const int TOTAL_POINTS = 1000000000;

std::mutex mutex;

struct Point {
    Point(double xx, double yy): x{xx}, y{yy} {}

    void print() { std::cout << "(" << x << ", " << y << ")\n"; }
    double x, y;
};

double nextRand() {
    // return a value in the range (-1, 1)

    return (rand() / double(RAND_MAX)) * 2 - 1;
}

void printPointsInQuadrant(const std::vector<Point> &points, int quadrant, int* totalQuadrantPoints) {
    // Counts and prints the number of points in each quadrant.

    std::cout << "Partitioning points for quadrant " << quadrant + 1 << std::endl;
    int one = 0, two = 0, three = 0, four = 0; 
    for( auto iter = points.begin(); iter != points.end(); iter++ ) {
        switch(quadrant){
            case 0:
                if( iter->x > 0 && iter->y > 0 ){
                    one++;
                    mutex.lock();
                    *totalQuadrantPoints += 1;
                    mutex.unlock();
                }
                break;
            case 1:
                if( iter->x > 0 && iter->y < 0 ){
                    two++;
                    mutex.lock();
                    *totalQuadrantPoints += 1;
                    mutex.unlock();
                }
                break;
            case 2:
                if( iter->x < 0 && iter->y < 0 ){
                    three++;
                    mutex.lock();
                    *totalQuadrantPoints += 1;
                    mutex.unlock();
                }
                break;
            case 3:
                if( iter->x < 0 && iter->y > 0 ){
                    four++;
                    mutex.lock();
                    *totalQuadrantPoints += 1;
                    mutex.unlock();
                }
                break;
        }
    }

    switch(quadrant){
        case 0:
            std::cout << "points in quadrant 1: " << one << std::endl;
            break;
        case 1:
            std::cout << "points in quadrant 2: " << two << std::endl;
            break;
        case 2:
            std::cout << "points in quadrant 3: " << three << std::endl;
            break;
        case 3:
            std::cout << "points in quadrant 4: " << four << std::endl;
            break;
    }
}

int countInCircle(const std::vector<Point> &points) {
    int inCircle = 0;
    for( auto iter = points.begin(); iter != points.end(); iter++ )
	if( sqrt( iter->x * iter->x + iter->y * iter->y ) <= 1.0 )  // is the point in the circle?
	    inCircle++;
    return inCircle;
}

void generatePoints(std::vector<Point> &points) {
    // Generates TOTAL_POINTS points.

    std::cout << "Will generate " << TOTAL_POINTS << std::endl;
    for( int i = 0; i < TOTAL_POINTS; i++ ) { 
	points.push_back(Point(nextRand(), nextRand()));
    }
    std::cout << "Finished generating points...\n";
}


int main() {
    srand(getpid());
    std::vector<Point> points;
    std::thread threads[4];
    int totalPoints = 0;

    std::cout << "It takes some time for this program to print its result.\n";

    generatePoints(points);

    for(int i = 0; i < 4; i++){
        threads[i] = std::thread(printPointsInQuadrant, points, i, &totalPoints);
    }    

    for(int i = 0; i < 4; i++){
        threads[i].join();
    }

    int inCircle = countInCircle(points);

    std:: cout << "Total Points: " << totalPoints << std::endl;

    std::cout << "PI = " << double(inCircle) * double(4.0) / double(TOTAL_POINTS) << std::endl;

    return 0;
}
