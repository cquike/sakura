/*
 * main.cc
 *
 *  Created on: 2013/10/21
 *      Author: kohji
 */

#include <iostream>
#include <unistd.h>
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
#include <xdispatch/dispatch>
#include <libsakura/sakura.h>
#include <casacore/ms/MeasurementSets/MeasurementSet.h>

namespace {
auto logger = log4cxx::Logger::getLogger("app");

void SerializedJob(int i) {
	std::cout << i << std::endl;
}

void ParallelJob(int i) {
	sleep(i % 3);
	xdispatch::main_queue().sync([=] {
		SerializedJob(i);
	});
}

void E2eReduce(int argc, char const* const argv[]) {
	LOG4CXX_INFO(logger, "Enter: E2eReduce");
	{
		auto group = xdispatch::group();
		for (int i = 0; i < 20; ++i) {
			group.async([=] {
				ParallelJob(i);
			});
		}
		group.wait(xdispatch::time_forever);
	}
	xdispatch::main_queue().async([=] {
		sleep(1);
		std::cout << "finished\n";
		exit(0);
	});
	LOG4CXX_INFO(logger, "Leave: E2eReduce");
}

void main_(int argc, char const* const argv[]) {
	LOG4CXX_INFO(logger, "start");
	sakura_Status result;
	xdispatch::main_queue().sync([&] {
		result = sakura_Initialize(nullptr, nullptr);
	});
	if (result == sakura_Status_kOK) {
		try {
			E2eReduce(argc, argv);
		} catch (...) {
			LOG4CXX_ERROR(logger, "Exception raised");
		}
		xdispatch::main_queue().sync([] {
			sakura_CleanUp();
		});
	} else {
		LOG4CXX_ERROR(logger, "Failed to initialize libsakura.");
	}
}

}

int main(int argc, char const* const argv[]) {
	::log4cxx::PropertyConfigurator::configure("config.log4j");
	xdispatch::global_queue().async([=] {
		main_(argc, argv);
	});
	xdispatch::exec(); // never returns
	return 0;
}
