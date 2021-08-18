#include <stdio.h>
#include <termios.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <poll.h>



#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>

#include <thread>
#include <chrono>
#include <memory>


using namespace std::chrono_literals;


enum Direction{
  LEFT,
  RIGHT,
  UP,
  DOWN
};



void moveCursor(std::ostream& os, int col, int row)
{
  os << "\033[" << row << ";" << col << "H";
}


int GoodRandom(int max, int min){
  time_t t;
  t = time(&t);
  srand(t);
  return rand() % max + min;
}


class Snake; 


class Point;


class GameObj{
  public:
    virtual void Draw(){}
    virtual bool IsHit(int _x, int _y){return false;}
    virtual bool IsFood(){return false;}
    virtual int X(){return -1;}
    virtual int Y(){return -1;}
};
bool point_is_busy(int x, int y, Snake& snake, std::vector<std::unique_ptr<GameObj>>& game_objects);
class Point : public GameObj
{
  private:
    int x = 0;
    int y = 0;
    const  char *sim = "⸚";


  public:
    Point(int _x, int _y, const char *_sim){
      x = _x;
      y = _y;
      sim = _sim;
    }

    Point(const Point& p){
      x = p.x;
      y = p.y;
      sim = p.sim;
    }

    bool operator==(Point& p){
      if (x == p.x && y == p.y && sim == p.sim){
        return true;
      }
      return false;
    }

    bool IsHit(int _x, int _y){
      if (_x == x && _y == y){return true;}
      return false;
    }

    bool IsFood(){return false;}

    int X(){
      return x;
    }

    int Y(){
      return y;
    }


    void Draw(){
      moveCursor(std::cout, x,y);
      std::cout << sim << std::endl;
    }

    void Draw(const char * _sim){
      moveCursor(std::cout, x,y);
      std::cout << _sim << std::endl;
    }

    void ColorDraw(const char * color){
      moveCursor(std::cout, x,y);
      std::cout << color << sim << "\033[0m" << std::endl;
    }


    void Move(int distance, Direction& direction){
      if (direction == RIGHT)    { x += distance; }
      else if (direction == LEFT){ x -= distance; }
      else if (direction == DOWN){ y += distance; }
      else if (direction == UP)  { y -= distance; }

    }


    void Delete(){
      moveCursor(std::cout, x,y);
      std::cout << " ";
    }

};



class Line : public GameObj
{
  private:
    std::vector<Point> points;

  public:
    Line(int x1, int y1, int x2, int y2, const char *sim){
      if (y1 != y2){  //vertical line
        if (y1 > y2){
          std::swap(y1, y2);
        }
        for (int p=y1; p<=y2; p++){
          points.push_back(Point(x1, p, sim));
        }
      }

      if (x1 != x2){ //horizontal line
        if (x1 > x2){
          std::swap(x1, x2);
        }
        for (int p=x1; p<=x2; p++){
          points.push_back(Point(p, y1, sim));
        }      
      }

    }
    void Draw(){
      for (Point p : points){
        p.Draw();
      }
    }

    bool IsHit(int _x, int _y){
      for(Point& line_point : points){
        if (line_point.X() == _x && line_point.Y() == _y){
          return true;
        }
      }
      return false;
    }
    bool IsFood(){return false;}

};

class Snake : public GameObj{
  private:
    Direction direction;
    std::vector<Point> points;
    bool need_grow = false;
    bool snake_died = false;

  public:
    Snake(Point& tail, int len, Direction _direction){
      direction = _direction;
      for (int i=0; i<len; i++){
        Point p(tail);
        p.Move(i, direction);
        points.push_back(p);
      }
    }

    void Draw(){
      for (Point p : points){
        p.Draw();
      }
    }

    void Move(Direction _direction){
      direction = _direction;
      Point p = points.back();
      p.Move(1, _direction);
      for (Point& snake_point : points){
        if (p == snake_point){
          Die();
          return;
        }
      }
      points.push_back(p);

      if (need_grow == false){
        points[0].Delete();
        points.erase(points.begin(), points.begin()+1);
      }
      else{need_grow = false;}


      points.back().Draw();

    }

    Point HeadPos(){
      return points.back();
    }


    Direction Dir(){
      return direction;
    }

    void Grow(){
      need_grow = true;
    }

    void Die(){
      snake_died = true;
      for (Point p : points){ 
        p.Draw("🪦");
      }
      moveCursor(std::cout, 1, 40);
      std::cout << "snake died";

    }

    bool Died(){
      return snake_died;
    }

    std::vector<Point> Points(){
      return points;
    }

    int Size(){
      return points.size();
    }

};

class Apple : public GameObj{
  private:
    Point apple = Point(0,0, "");

  public:
    Apple(int max_x, int min_x, int max_y, int min_y, std::vector<std::unique_ptr<GameObj>>& game_objects, Snake& snake){
      while (true)
      {
        int x = GoodRandom(max_x, min_x);
        int y = GoodRandom(max_y, min_y);
        if (point_is_busy(x,y, snake, game_objects) == false){
          apple = Point(x,y, "$");
          break;
        }
      }
    }

    void Draw(){
      
      apple.ColorDraw("\033[1;32m");
    }

    bool IsHit(int _x, int _y){
      if (apple.X()==_x && apple.Y()==_y){
        return true;}
      return false;
    }
    bool IsFood(){return true;}
    
};

bool CheckHit(Snake& snake, std::vector<std::unique_ptr<GameObj>>& game_objects){
  Point head_pos = snake.HeadPos();
  for (int i=0; i<game_objects.size(); i++){
    if(game_objects[i]->IsHit(head_pos.X(), head_pos.Y())){
      if (game_objects[i]->IsFood()){
        snake.Grow();
        game_objects.erase(game_objects.begin()+i);
        return true;
      }
      else{
        snake.Die();
        return false;
      }
      return false;
    }
  }
  return false;
}


bool point_is_busy(int x, int y, Snake& snake, std::vector<std::unique_ptr<GameObj>>& game_objects){
  std::vector<Point> points = snake.Points();

  for (Point& p : points){
    if (p.X() == x && p.Y() == y){
      return true;
    }
  }
  for (std::unique_ptr<GameObj>& obj : game_objects){
    if (obj->IsHit(x, y)){
      return true;
    }
  }



  return false;
}

void PrintScore(Snake& snake){
  moveCursor(std::cout, 45, 2);
  std::cout << "\033[1;32m" << "Score: " << snake.Size() << "\033[0m" << std::endl;
}

Point p1(10,10,"🐗");
Point p2(1,2,"☻");
struct termios term;



int main(){
	// initscr();			/* Start curses mode 		*/
	// raw();				/* Line buffering disabled	*/
	// keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	// noecho();	
  // curs_set(0);

  // int dir = getch();

  //stop echo

	WINDOW* w = initscr();			/* Start curses mode 		*/
cbreak();
nodelay(w, TRUE);
	raw();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();	
  curs_set(0);

  int dir = getch();
  if (dir == ERR){
    dir = 258;
  }


  tcgetattr(fileno(stdin), &term);
  term.c_lflag &= ~ECHO;
  tcsetattr(fileno(stdin), 0, &term);

  

  std::vector<std::unique_ptr<GameObj>> game_objects;
  std::unique_ptr<GameObj> obj1 = std::make_unique<GameObj>();

  auto line1 = std::make_unique<Line>(1,2, 100,2, "+");
  auto line2 = std::make_unique<Line>(100,2, 100,50, "+");
  auto line3 = std::make_unique<Line>(100,50, 1,50, "+");
  auto line4 = std::make_unique<Line>(1,50, 1,2, "+");   
  line1->Draw();
  line2->Draw();
  line3->Draw();
  line4->Draw();

  auto line5 = std::make_unique<Line>(50,40, 70,40, "+");
  auto line6 = std::make_unique<Line>(40,10, 40,40, "+");
  auto line7 = std::make_unique<Line>(70,10, 70,40, "+");
  auto line8 = std::make_unique<Line>(14,40, 14,5, "+");   
  auto line9 = std::make_unique<Line>(30,10, 30,27, "+");
  auto line10 = std::make_unique<Line>(20,30, 49,30, "+");   
  auto line11 = std::make_unique<Line>(70,10, 97,10, "+");
  auto line12 = std::make_unique<Line>(60,5, 60,25, "+");   
  auto line13 = std::make_unique<Line>(30,32, 30,50, "+");
  auto line14 = std::make_unique<Line>(80,40, 100,40, "+");
  line5->Draw();
  line6->Draw();
  line7->Draw();
  line8->Draw();
  line9->Draw();
  line10->Draw();
  line11->Draw();
  line12->Draw();
  line13->Draw();
  line14->Draw();

  game_objects.push_back(std::move(line1));
  game_objects.push_back(std::move(line2));
  game_objects.push_back(std::move(line3));
  game_objects.push_back(std::move(line4));
  game_objects.push_back(std::move(line5));
  game_objects.push_back(std::move(line6));
  game_objects.push_back(std::move(line7));
  game_objects.push_back(std::move(line8));
  game_objects.push_back(std::move(line9));
  game_objects.push_back(std::move(line10));
  game_objects.push_back(std::move(line11));
  game_objects.push_back(std::move(line12));
  game_objects.push_back(std::move(line13));
  game_objects.push_back(std::move(line14));
  Point snake_begin(5,5,"o");
  Snake snake(snake_begin, 1, RIGHT);
  snake.Draw();

  std::unique_ptr<Apple> apple1 = std::make_unique<Apple>(98, 3, 48, 3, game_objects, snake);
  // Apple apple2(10, 1, 10, 1);
  // Apple apple3(10, 1, 10, 1);
  // Apple apple4(10, 1, 10, 1);

  apple1->Draw();
  // apple2.Draw();
  // apple3.Draw();
  // apple4.Draw();


  game_objects.push_back(std::move(apple1));
  // game_objects.push_back(&apple2);
  // game_objects.push_back(&apple3);
  // game_objects.push_back(&apple4);


  // initscr();
  // timeout(1);




 // int dir = 259;
  int prev_dir = dir;

  while (true){
    // sleep(1);

    // int dir = rand() % 4 + 1;
 
    dir = getch();
    if (dir == ERR){
      dir = prev_dir;
    }
    prev_dir = dir;
    if (dir == 259 || dir == 258){
      std::this_thread::sleep_for(200ms);
    }
    else{
      std::this_thread::sleep_for(100ms);
    }


    if (dir == 259 && snake.Dir()!=DOWN){
      snake.Move(UP);
    }
    else if (snake.Dir() == DOWN && dir == 259)
    {
      snake.Move(DOWN);
    }
    
    

    if (dir == 258 && snake.Dir()!=UP){
      snake.Move(DOWN);
    }
    else if (snake.Dir() == UP && dir == 258)
    {
      snake.Move(UP);
    }



    if (dir == 261 && snake.Dir()!=LEFT){
      snake.Move(RIGHT);
    }
    else if (snake.Dir() == LEFT && dir == 261)
    {
      snake.Move(LEFT);
    }




    if (dir == 260 && snake.Dir()!=RIGHT){
      snake.Move(LEFT);
    }
    else if (snake.Dir() == RIGHT && dir == 260)
    {
      snake.Move(RIGHT);
    }


    if (dir == 27){
      break;
    }
    bool need_new_apple = CheckHit(snake, game_objects);

    if (snake.Died()){
      break;
    }

    if (need_new_apple){
      std::unique_ptr<Apple> apple = std::make_unique<Apple>(50, 3, 47, 3, game_objects, snake);
      apple->Draw();
      game_objects.push_back(std::move(apple));
      PrintScore(snake);
    }
  }







  //enable echo
  term.c_lflag |= ECHO;
  tcsetattr(fileno(stdin), 0, &term);

  curs_set(1);
  return 0;

}




      // Apple apple(50, 3, 47, 3, game_objects, snake);
      // apple.Draw();
      // game_objects.push_back(&apple);
