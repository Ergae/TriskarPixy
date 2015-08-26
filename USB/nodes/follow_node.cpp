
#include "ch.h"
#include "hal.h"
#include "r2p/Middleware.hpp"
#include <r2p/msg/pixy.hpp>
#include <r2p/msg/motor.hpp>
#include <r2p/msg/follow.hpp>

// Robot parameters
#define _L        0.160f    // Wheel distance [m]
#define _R        0.035f    // Wheel radius [m]
#define _MAX_DTH  52.0f     // Maximum wheel angular speed [rad/s]

#define _m1_R     (-1.0f / _R)
#define _mL_R     (-_L / _R)
#define _C60_R    (0.500000000f / _R)   // cos(60°) / R
#define _C30_R    (0.866025404f / _R)   // cos(30°) / R

#define _TICKS 64.0f
#define _RATIO 29.0f
#define _PI 3.14159265359f

#define R2T(r) (_TICKS * _RATIO)/(r * 2 * _PI)
#define T2R(t) (t / R2T)

#define M2T(m) (m * _TICKS * _RATIO)/(2 * _PI * _R)
#define T2M(t) (t / _M2TICK)

template<typename T> static inline T clamp(T min, T value, T max) {
	return (value < min) ? min : ((value > max) ? max : value);
}

static int follow=0;

msg_t follow_node(void *arg) {
	//Routine

	r2p::Node vel_node("speedpub", false);
	r2p::Publisher<r2p::Speed3Msg> vel_pub;
	bool speed_first_time = true;

	r2p::Node pixy_node("pixy", false);
	r2p::Subscriber<r2p::PixyMsg, 5> pixy_sub;
	bool pixy_first_time = true;

	r2p::Node shell_node("follow", false);
	r2p::Subscriber<r2p::FollowMsg, 5> shell_sub;
	bool shell_first_time = true;

	r2p::PixyMsg * pixy_msgp;
	r2p::Speed3Msg * speed_msgp;
	r2p::FollowMsg * follow_msgp;
	if (pixy_first_time) {
		pixy_node.subscribe(pixy_sub, "pixy");
		pixy_first_time = false;
	}
	if (speed_first_time) {
		vel_node.advertise(vel_pub, "speed3", r2p::Time::INFINITE);
		speed_first_time = false;
	}
	if (shell_first_time) {
		shell_node.subscribe(shell_sub, "follow");
		shell_first_time = false;
	}


	float x = 0;
	float y = 0;
	float w = 0;
	pixy_node.set_enabled(true);
	vel_node.set_enabled(true);
	shell_node.set_enabled(true);
	//chprintf(chp, "BOOOOOOM\n");

	for(;;){

		pixy_node.set_enabled(true);
		vel_node.set_enabled(true);
		shell_node.set_enabled(true);

		if(shell_sub.fetch(follow_msgp))
		{
			follow = follow_msgp->follow;
			shell_sub.release(*follow_msgp);
		}

		if(follow){
			x = 0;
			y = 0;
			w = 0;
			int timeout = 0;

			//clean trash
			for(int trash=0;trash<5;trash++){
				r2p::Thread::sleep(r2p::Time::ms(5));
				if(pixy_sub.fetch(pixy_msgp)){
					pixy_sub.release(*pixy_msgp);
				}
			}

			while(!pixy_sub.fetch(pixy_msgp) && timeout <1000)  {
				r2p::Thread::sleep(r2p::Time::ms(5));
				timeout++;
			}
			if(timeout<1000){
				//chprintf(chp, "Height: %d \n", pixy_msgp->height);
				//temporary collision checking
				if(pixy_msgp->height<90){
					//no collision
					if(pixy_msgp->x>200){
						//chprintf(chp, "Rotating Right\n");
						x = -0.2;
						y = 0;
						w = 0.1;
					}else if(pixy_msgp->x<120){
						//chprintf(chp, "Rotating Left\n");
						x = -0.2;
						y = 0;
						w = -0.2;
					}else{
						//chprintf(chp, "Going Ahead\n");
						x=-0.4;
						y = 0;
						w = 0;
					}
				}
				else {
					//chprintf(chp, "Object Found\n");
					if(pixy_msgp->x>170){
						//chprintf(chp, "Rotating Right\n");
						x = -0.2;
						y = 0;
						w = 0.1;
					}else if(pixy_msgp->x<150){
						//chprintf(chp, "Rotating Left\n");
						x = -0.2;
						y = 0;
						w = -0.2;
					}else{
						//chprintf(chp, "Object Centered\n");
						x=0;
						y = 0;
						w = 0;
					}
				}
				pixy_sub.release(*pixy_msgp);
			}
			else{
				//chprintf(chp, "Searching\n");
				x=0;
				y = 0;
				w = 1;
			}

			//moving
			// Wheel angular speeds
			const float dthz123 = _mL_R * w;
			const float dx12 = _C60_R * y;
			const float dy12 = _C30_R * x;

			float dth1 = dx12 - dy12 + dthz123;
			float dth2 = dx12 + dy12 + dthz123;
			float dth3 = _m1_R * y + dthz123;

			// Motor setpoints
			if (vel_pub.alloc(speed_msgp)) {
				speed_msgp->value[0] = (int16_t) clamp(-_MAX_DTH, dth1, _MAX_DTH);
				speed_msgp->value[1] = (int16_t) clamp(-_MAX_DTH, dth2, _MAX_DTH);
				speed_msgp->value[2] = (int16_t) clamp(-_MAX_DTH, dth3, _MAX_DTH);
				vel_pub.publish(*speed_msgp);
			}

		}
		shell_node.set_enabled(false);
		vel_node.set_enabled(false);
		pixy_node.set_enabled(false);

		r2p::Thread::sleep(r2p::Time::ms(200));
	}
	return CH_SUCCESS;
}




