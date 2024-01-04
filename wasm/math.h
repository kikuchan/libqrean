#pragma once

#include "stdint.h"

#define NAN (1.0f / 0.0f)
#define M_PI 3.1415927565f
#define M_PI_2 (M_PI / 2.0f)
#define M_PI_4 (M_PI / 4.0f)

double sqrt(double f);

#ifdef USE_JS_MATH
__attribute__((import_module("env"), import_name("roundf")))
extern float roundf(float f);
__attribute__((import_module("env"), import_name("round")))
extern double round(double f);
__attribute__((import_module("env"), import_name("pow")))
extern double pow(double f, double n);
__attribute__((import_module("env"), import_name("atan2"))) 
extern double atan2(double y, double x);
__attribute__((import_module("env"), import_name("cos")))
extern double cos(double f);
__attribute__((import_module("env"), import_name("sin")))
double sin(double f);
__attribute__((import_module("env"), import_name("fmod")))
extern double fmod(double n, double m);
#else
double atan2(double y, double x);
double pow(double f, double n);
#endif

float atan2f(float y, float x);
double fabs(double n);
int isnan(float f);
int abs(int n);
double floor(double f);
