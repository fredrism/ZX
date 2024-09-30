#pragma once
#include <chrono>
#include <iostream>

class ScopedProfiler {
public:
	ScopedProfiler() {
		this->m_start = std::chrono::high_resolution_clock::now();
	}

	~ScopedProfiler() {
		const auto end = std::chrono::high_resolution_clock::now();

		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count() << "ms" << std::endl;
	}

private:
	std::chrono::high_resolution_clock::time_point m_start;
};

class FPSProfiler {
public:
	FPSProfiler() {
		this->m_start = std::chrono::high_resolution_clock::now();
	}

	~FPSProfiler() {
		const auto end = std::chrono::high_resolution_clock::now();

		std::cout << 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count() << "fps" << std::endl;
	}

private:
	std::chrono::high_resolution_clock::time_point m_start;
};