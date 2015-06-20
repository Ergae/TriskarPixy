#include <stdint.h> // atof()

#include "ch.h"
#include "hal.h"

#include "usbcfg.h"

#include <r2p/Middleware.hpp>
#include <r2p/node/led.hpp>

#include "nodes/shell_node.hpp"
#include "nodes/pixy_node.hpp"


static WORKING_AREA(wa_info, 1024);
static r2p::RTCANTransport rtcantra(RTCAND1);

RTCANConfig rtcan_config = { 1000000, 100, 60 };

r2p::Middleware r2p::Middleware::instance(MODULE_NAME, "BOOT_"MODULE_NAME);


/*
 * Application entry point.
 */
extern "C" {
int main(void) {

	halInit();
	chSysInit();

	r2p::Middleware::instance.initialize(wa_info, sizeof(wa_info), r2p::Thread::LOWEST);
	rtcantra.initialize(rtcan_config);
	r2p::Middleware::instance.start();

	r2p::ledpub_conf ledpub_conf = { "led", 1 };
	r2p::Thread::create_heap(NULL, THD_WA_SIZE(512), NORMALPRIO, r2p::ledpub_node, &ledpub_conf);

	r2p::ledsub_conf ledsub_conf = {"led"};
	r2p::Thread::create_heap(NULL, THD_WA_SIZE(512), NORMALPRIO, r2p::ledsub_node, &ledsub_conf);

	r2p::Thread::create_heap(NULL, THD_WA_SIZE(512), NORMALPRIO, shell_node, NULL);
	r2p::Thread::create_heap(NULL, THD_WA_SIZE(512), NORMALPRIO, pixy_node, NULL);

	for (;;) {
		r2p::Thread::sleep(r2p::Time::ms(500));
	}

	return CH_SUCCESS;
}
}

