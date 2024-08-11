#include <iostream>
#include "thread_timer.hpp"

using namespace std;

int main() {
	int cnt = 0;
	
	ThreadTimer t0(1000ms, [&](){ 
		cout << "1000ms Elapsed!, cnt = " << ++cnt << "\n"; 
		});

	t0.start();
 
	// wait until cnt == 10
	while (cnt < 10) {
		// do something...
	}

	return 0;
}