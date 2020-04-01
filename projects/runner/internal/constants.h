#pragma once


namespace mff::constants {

#ifdef NDEBUG
const bool kDEBUG = false;
#else
const bool kDEBUG = true;
#endif

const bool kVULKAN_DEBUG = kDEBUG;

}