#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <mutex>
#include <cmath>
#include <memory>
#include <condition_variable>
#include <ncurses.h>

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
std::mutex mtx2;
std::condition_variable cv;
std::condition_variable cv2;
bool isBallHit = false;

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
	bool isBallMoving;

public:

	Ball(int xPosition, int yPosition)
	{
		horizontalShift = 1;
		verticalShift = 1;
		this->xPosition = xPosition;
		this->yPosition = yPosition;
		SetBallMoving(true);
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

	int GetHorizontalShift()
	{
		return horizontalShift;
	}

	int GetVerticalShift()
	{
		return verticalShift;
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

	void DisplaceBall()
	{
		xPosition += horizontalShift;
		yPosition += verticalShift;
	}

	void SetBallMoving(bool isBallMoving)
	{
		this->isBallMoving = isBallMoving;
	}

	void SetBallVelocity(int velocity)
	{
		this->velocity = velocity;
	}

	void BallCollision()
	{
		//while(runningLoop && isBallMoving)
		//{
			// lustrzane odbicia kulek
			if(GetXPosition() == 0 || GetXPosition() == rows - 1) horizontalShift = -horizontalShift;
			if(GetYPosition() == 0 || GetYPosition() == columns - 1) verticalShift = -verticalShift;
			DisplaceBall();
			//std::this_thread::sleep_for(std::chrono::milliseconds(velocity));
		//}
	}

	std::thread MotionThread()
	{
		return std::thread(&Ball::BallCollision, this);
	}
};

std::vector<Ball*> balls;
std::vector<std::thread> threadsOfBalls;

bool ShouldBallStop()
{
	return !isBallHit && runningLoop;
}

int GetAbsoluteXLength(int i, int j)
{
	return sqrt((balls[i]->GetXPosition() - balls[j]->GetXPosition()) * (balls[i]->GetXPosition() - balls[j]->GetXPosition()));
}

int GetAbsoluteYLength(int i, int j)
{
	return sqrt((balls[i]->GetYPosition() - balls[j]->GetYPosition()) * (balls[i]->GetYPosition() - balls[j]->GetYPosition()));
}

bool BallInMutex(int nrOfBall)
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

void BallThreadFunction(int nrOfBall)
{
	bool ballLeavingMutex = false;
	int delay = 0;

	while(runningLoop)
	{
		balls[nrOfBall]->BallCollision();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		if(BallInMutex(nrOfBall))
		{
			/*if(ballLeavingMutex) continue;
			if(delay < 1)
			{
				delay++;
				continue;
			}
*/
			std::unique_lock<std::mutex> lock2(mtx2);
			isBallHit = true;
			while(runningLoop)
			{
				cv2.wait(lock2);
			}
			mtx2.unlock();

			std::unique_lock<std::mutex> lock1(mtx);

			while(!isBallHit && runningLoop)
			{
				cv.wait(lock1);
			}
			isBallHit = false;
			lock1.unlock();
		}
		else delay = 0;
	}
}

void CreateBall()
{
	int i = 0;
	while(runningLoop)
	{
		getmaxyx(stdscr, rows, columns);
		balls.push_back(new Ball(rows / 2, columns / 2));
		//threadsOfBalls.push_back(balls.back()->MotionThread());
		threadsOfBalls.push_back(std::thread(BallThreadFunction, i));
		i++;
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}
}

void TerminateThreadsOfBalls()
{
	for (int i = 0; i < threadsOfBalls.size(); ++i)
	{
		threadsOfBalls[i].join();
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
		cv2.notify_all();
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