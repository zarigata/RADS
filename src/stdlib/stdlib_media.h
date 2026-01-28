#ifndef RADS_STDLIB_MEDIA_H
#define RADS_STDLIB_MEDIA_H

#include "../core/interpreter.h"

// Media Module registration
void stdlib_media_register(void);

// Native functions
Value native_media_image_info(struct Interpreter* interp, int argc, Value* args);
Value native_media_audio_info(struct Interpreter* interp, int argc, Value* args);
Value native_media_convert(struct Interpreter* interp, int argc, Value* args);

#endif // RADS_STDLIB_MEDIA_H
