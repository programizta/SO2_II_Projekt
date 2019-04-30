#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <mutex>
#include <cmath>
#include <memory>
#include <condition_variable>
#include <ncurses.h>
#include <exception> 

thread_local std::mt19937 gen{ std::random_device{}() };

// szablonik funkcji do generowania liczb
// zgodnie z rozkładem jednostajnym
template<typename var>
var random(var min, var max) {
	using dist = std::conditional_t<
		std::is_integral<var>::value,
		std::uniform_int_distribution<var>,
		std::uniform_real_distribution<var>
	>;
	return dist{ min, max }(gen);
}

int rows = 0;
int columns = 0;
bool runningLoop = true;
std::mutex mtx;
std::condition_variable cv;
int indexOfSecondBall;
int resultantVerticalShift;
int resultantHorizontalShift;
int resultantVelocity;

class Ball
{
	int horizontalShift;
	int verticalShift;
	int xPosition;
	int yPosition;
	int direction;
	int xSpeed;
	int ySpeed;
	int velocity;
	bool exitThread;

public:

	Ball(int xPosition, int yPosition)
	{
		exitThread = false;
		horizontalShift = 1;
		verticalShift = 1;
		this->xPosition = xPosition;
		this->yPosition = yPosition;
		InitializeDirection(random(1, 8));
		InitializeSpeed(random(10, 300));
	}

	~Ball() { }

	void InitializeDirection(int choice)
	{
		switch(choice)
		{
			case 1: MoveLeft(); break;
			case 2: MoveUpperLeft(); break;
			case 3: MoveLowerLeft(); break;
			case 4: MoveRight(); break;
			case 5: MoveUpperRight(); break;
			case 6: MoveLowerRight(); break;
			case 7: MoveUp(); break;
			case 8: MoveDown(); break;
		}
	}

	void InitializeSpeed(int choice)
	{
		if(choice % 3 == 0) velocity = 50;
		else if(choice % 3 == 1) velocity = 150;
		else velocity = 200;
	}

	int GetXPosition()
	{
		return xPosition;
	}

	int GetYPosition()
	{
		return yPosition;
	}

	void SetXPosition(int xPosition)
	{
		this->xPosition = xPosition;
	}

	void SetYPosition(int yPosition)
	{
		this->yPosition = yPosition;
	}

	int GetHorizontalShift()
	{
		return horizontalShift;
	}

	int GetVerticalShift()
	{
		return verticalShift;
	}

	int GetVelocity()
	{
		return velocity;
	}

	void SetVelocity(int velocity)
	{
		this->velocity = velocity;
	}

	void SetHorizontalShift(int horizontalShift)
	{
		this->horizontalShift = horizontalShift;
	}

	void SetVerticalShift(int verticalShift)
	{
		this->verticalShift = verticalShift;
	}

	void StopTheBall()
	{
		this->exitThread = false;
	}

	bool CheckIfBallStopped()
	{
		return exitThread;
	}

	void MoveRight()
	{
		horizontalShift = 1;
		verticalShift = 0;
	}

	void MoveUpperRight()
	{
		horizontalShift = 1;
		verticalShift = -1;
	}

	void MoveLowerRight()
	{
		horizontalShift = 1;
		verticalShift = 1;
	}

	void MoveLeft()
	{
		horizontalShift = -1;
		verticalShift = 0;
	}

	void MoveUpperLeft()
	{
		horizontalShift = -1;
		verticalShift = -1;		
	}

	void MoveLowerLeft()
	{
		horizontalShift = -1;
		verticalShift = 1;
	}

	void MoveUp()
	{
		horizontalShift = 0;
		verticalShift = -1;
	}

	void MoveDown()
	{
		horizontalShift = 0;
		verticalShift = 1;
	}

	void DisplaceBall(double x)
	{
		xPosition += (int)(x*horizontalShift);
		yPosition += (int)(x*verticalShift);
	}

	void BallCollisionWithWall()
	{
		// lustrzane odbicia kulek
		if(GetXPosition() <= 0 || GetXPosition() >= rows - 1) horizontalShift = -horizontalShift;
		if(GetYPosition() <= 0 || GetYPosition() >= columns - 1) verticalShift = -verticalShift;
		if(velocity == 50) DisplaceBall(1.0);
		//else if(velocity == 150) DisplaceBall(2.0);
		else DisplaceBall(1.0);
	}
};

std::vector<Ball*> balls;
std::vector<std::thread> threadsOfBalls;

bool DidBallsHit(int nrOfBall)
{
	int xCoord = balls[nrOfBall]->GetXPosition();
	int yCoord = balls[nrOfBall]->GetYPosition();

	for (int i = 0; i < balls.size(); ++i)
	{
		if((balls[i]->GetXPosition() - xCoord == 0) && (balls[i]->GetYPosition() - yCoord == 0) && i != nrOfBall)
		{
			return true;
		}
	}
	return false;
}

int GetIndexOfSecondBall(int nrOfBall)
{
	int n;
	int xCoord = balls[nrOfBall]->GetXPosition();
	int yCoord = balls[nrOfBall]->GetYPosition();

	for (int i = 0; i < balls.size(); ++i)
	{
		if((balls[i]->GetXPosition() - xCoord == 0) && (balls[i]->GetYPosition() - yCoord == 0) && i != nrOfBall)
		{
			n = i;
			return i;
		}
	}
	return n;
}

int GetResultantHorizontalShift(int i, int j)
{
	int horizontalShiftOfFirstBall = balls[i]->GetHorizontalShift();
	int horizontalShiftOfSecondBall = balls[j]->GetHorizontalShift();
	
	if(horizontalShiftOfFirstBall == horizontalShiftOfSecondBall) return horizontalShiftOfFirstBall;
	else if((horizontalShiftOfFirstBall == 0 && horizontalShiftOfSecondBall == 1) || (horizontalShiftOfFirstBall == 1 && horizontalShiftOfSecondBall == 0))
	{
		return 1;
	}
	else if((horizontalShiftOfFirstBall == 0 && horizontalShiftOfSecondBall == -1) || (horizontalShiftOfFirstBall == -1 && horizontalShiftOfSecondBall == 0))
	{
		return -1;
	}
	else return 0;
}

int GetResultantVerticalShift(int i, int j)
{
	int verticalShiftOfFirstBall = balls[i]->GetVerticalShift();
	int verticalShiftOfSecondBall = balls[j]->GetVerticalShift();

	if(verticalShiftOfFirstBall == verticalShiftOfSecondBall) return verticalShiftOfFirstBall;
	else if((verticalShiftOfFirstBall == 0 && verticalShiftOfSecondBall == 1) || (verticalShiftOfFirstBall == 1 && verticalShiftOfSecondBall == 0))
	{
		return 1;
	}
	if((verticalShiftOfFirstBall == 0 && verticalShiftOfSecondBall == -1) || (verticalShiftOfFirstBall == -1 && verticalShiftOfSecondBall == 0))
	{
		return -1;
	}
	else return 0;
}

int GetResultantVelocity(int i, int j)
{
	int horizontalShiftOfFirstBall = balls[i]->GetHorizontalShift();
	int horizontalShiftOfSecondBall = balls[j]->GetHorizontalShift();
	int verticalShiftOfFirstBall = balls[i]->GetVerticalShift();
	int verticalShiftOfSecondBall = balls[j]->GetVerticalShift();

	if(horizontalShiftOfFirstBall == horizontalShiftOfSecondBall && verticalShiftOfFirstBall == verticalShiftOfSecondBall) return 50;
	else if(horizontalShiftOfFirstBall != horizontalShiftOfSecondBall && verticalShiftOfFirstBall == verticalShiftOfSecondBall) return 150;
	else if(horizontalShiftOfFirstBall == horizontalShiftOfSecondBall && verticalShiftOfFirstBall != verticalShiftOfSecondBall) return 50;
	else return 150;
}

void BallThreadFunction(int nrOfBall)
{
	int velocity;

	while(runningLoop)
	{
		balls[nrOfBall]->BallCollisionWithWall();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		std::unique_lock<std::mutex> lock(mtx);

		if(DidBallsHit(nrOfBall))
		{
			indexOfSecondBall = GetIndexOfSecondBall(nrOfBall);

			resultantVerticalShift = GetResultantVerticalShift(nrOfBall, indexOfSecondBall);
			resultantHorizontalShift = GetResultantHorizontalShift(nrOfBall, indexOfSecondBall);
			velocity = GetResultantVelocity(nrOfBall, indexOfSecondBall);
			balls[nrOfBall]->SetVelocity(velocity);
			balls[nrOfBall]->SetVerticalShift(resultantVerticalShift);
			balls[nrOfBall]->SetHorizontalShift(resultantHorizontalShift);
			balls[indexOfSecondBall]->StopTheBall();

			balls[indexOfSecondBall]->SetXPosition(1000);
			balls[indexOfSecondBall]->SetYPosition(1000);

			/*cv.wait(lock);*/
			
			//std::terminate();
			threadsOfBalls[indexOfSecondBall].join();
			lock.unlock();
		}

		

		// zakończenie wątku, nie wait
	}
}

void CreateBall()
{
	int i = 0;
	while(i < 15)
	{
		getmaxyx(stdscr, rows, columns);
		balls.push_back(new Ball(rows / 2, columns / 2));
		threadsOfBalls.push_back(std::thread(BallThreadFunction, i));
		i++;
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}
}

void TerminateThreadsOfBalls()
{
	for (int i = 0; i < threadsOfBalls.size(); ++i)
	{
		if(balls[i]->CheckIfBallStopped() == false) continue;
		else threadsOfBalls[i].join();
	}
}

void PressKeyToEnd()
{
	// zakończ program gdy wciśnięto klawisz 'q'
	while(runningLoop)
	{
		cbreak();
		noecho();
		char key = getch();
		if (key == 'q') runningLoop = false;
		else std::this_thread::sleep_for(std::chrono::milliseconds(20));
		cv.notify_all();
	}
}

void RenderScene()
{
	while(runningLoop)
	{
		clear();
		for (int i = 0; i < balls.size(); ++i)
		{
			mvprintw(balls[i]->GetXPosition(), balls[i]->GetYPosition(), "o");
		}

		refresh();
		// odświeżanie
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

int main(int argc, char const *argv[])
{
	initscr();
	curs_set(0);
	std::thread scene(RenderScene);
	std::thread createBalls(CreateBall);
	std::thread exitProgram(PressKeyToEnd);

	scene.join();
	createBalls.join();
	TerminateThreadsOfBalls();
	exitProgram.join();
	endwin();
	return 0;
}