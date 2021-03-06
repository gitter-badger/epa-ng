#include "Epatest.hpp"

#include <chrono>
#include <thread>

#include "util/Timer.hpp"

TEST(Timer, pause)
{
  Timer<> t;
  t.start();

  std::this_thread::sleep_for(std::chrono::milliseconds(2));

  t.pause();

  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  t.resume();
  t.stop();

  ASSERT_GT(t.average(), 2000.0);
  ASSERT_LT(t.average(), 3000.0);

}

TEST(Timer, construct_from_avg)
{
  Timer<> t;
  t.start();

  std::this_thread::sleep_for(std::chrono::milliseconds(2));

  t.stop();

  Timer<> tt(t.avg_duration());

  ASSERT_DOUBLE_EQ(tt.average(), t.average());

}