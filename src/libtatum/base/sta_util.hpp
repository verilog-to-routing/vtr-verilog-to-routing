#pragma once

float time_sec(struct timespec start, struct timespec end);

void print_histogram(const std::vector<float>& values, int nbuckets);
