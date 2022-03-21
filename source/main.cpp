#include <ctime>
#include <iostream>
#include <thread>
#include <exception>
#include <atomic>

#ifdef __linux__
	#include <sys/prctl.h>
#endif

#include <GarrysMod/Lua/Interface.h>

#define _MODULE_VERSION_ "0.0.1"

std::atomic<std::time_t> srvtime = ATOMIC_VAR_INIT(0);
std::atomic_ushort killtime = ATOMIC_VAR_INIT(60);
std::atomic_bool flag = ATOMIC_VAR_INIT(true);
std::atomic_bool restart = ATOMIC_VAR_INIT(false);
std::atomic_bool paused = ATOMIC_VAR_INIT(false);

void watchdog()
{
#ifdef __linux__
	prctl(PR_SET_NAME,"antifreeze\0",NULL,NULL,NULL);
#endif
	std::cout << "[Hang2Kill] Watchdog starting up" << std::endl;

	unsigned short timeout = 0;

	while(flag){
		std::this_thread::sleep_for(std::chrono::seconds(1));
		//std::cout << "srvrtime (thread) is " << srvrtime << "\n";
		if(srvtime == 0 || paused) {
			//do nothing
		} else if(restart){
				std::cout << "[Hang2Kill] Manual restart requested, killing process..." << std::endl;
				exit(0);
		} else if(srvtime >= (std::time(nullptr)) - 2) {
			if (timeout != 0) {
				timeout = 0;
				std::cout << "[Hang2Kill] Server caught back up!" << std::endl;
			}
		} else {
			timeout++;
			std::cout << "[Hang2Kill] Hang Detected! (" << timeout << ")" << std::endl;
			if (timeout == killtime) {
				std::cout << "[Hang2Kill] Server hang timeout! Killing process..." << std::endl;
				exit(139);
			} else if (timeout > killtime + 5) {
				std::cout << "[Hang2Kill] Its not... its not shutting down!" << std::endl;
				std::terminate();
			}
		}
	}

	std::cout << "[Hang2Kill] Watchdog shutdown. Please change map / restart server to start it again if you wish" << std::endl;
}
std::thread hang_detector(watchdog);

LUA_FUNCTION(RestartServer)
{
	restart.store(true, std::memory_order_release);
	return 0;
}

LUA_FUNCTION(SetTimeout)
{
	killtime.store(static_cast<unsigned short>(LUA->CheckNumber(1)), std::memory_order_release);
	return 0;
}

LUA_FUNCTION(WatchDogPing)
{
	srvtime.store(std::time(nullptr), std::memory_order_release);
	//std::cout << "srvrtime (WatchDogPing) is " << srvrtime << "\n";
	return 0;
}

LUA_FUNCTION(WatchDogStop)
{
	flag.store(false, std::memory_order_release);
	hang_detector.join();
	return 0;
}

LUA_FUNCTION(WatchDogSetPaused)
{
	paused.store(LUA->GetBool(1), std::memory_order_release);
	return 0;
}

GMOD_MODULE_OPEN()
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->CreateTable();
		LUA->PushString(_MODULE_VERSION_);
		LUA->SetField(-2, "version");

		LUA->PushCFunction(WatchDogStop);
		LUA->SetField(-2, "WatchdogStop");

		LUA->PushCFunction(SetTimeout);
		LUA->SetField(-2, "SetTimeout");

		LUA->PushCFunction(RestartServer);
		LUA->SetField(-2, "RestartServer");

		LUA->PushCFunction(WatchDogSetPaused);
		LUA->SetField(-2, "WatchDogSetPaused");
	LUA->SetField(-2, "hang2kill");

	LUA->GetField(-1, "timer");
		LUA->GetField(-1, "Create");
			LUA->PushString("hang2kill_watchdog");
			LUA->PushNumber(1);
			LUA->PushNumber(0);
			LUA->PushCFunction(WatchDogPing);
		LUA->Call(4, 0);
	LUA->Pop(2);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	flag.store(false, std::memory_order_release);
	hang_detector.join();

	return 0;
}