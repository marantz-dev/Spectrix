#pragma once
#include <JuceHeader.h>

template <typename TYPE, size_t SIZE> class CircularBuffer {
    public:
      CircularBuffer() : writeIndex(0), readIndex(0), count(0) {}

      void push(float sample) {
            buffer[writeIndex] = sample;
            writeIndex = (writeIndex + 1) % SIZE;

            if(count < SIZE)
                  ++count;
            else
                  readIndex = (readIndex + 1) % SIZE;
      }

      float pop() {
            if(count == 0)
                  return 0.0f; // empty

            float sample = buffer[readIndex];
            readIndex = (readIndex + 1) % SIZE;
            --count;
            return sample;
      }

      float operator[](size_t index) const {
            jassert(index < count);
            return buffer[(readIndex + index) % SIZE];
      }

      float getFirstElement() const { return buffer[readIndex]; }

      size_t size() const { return count; }

    private:
      std::array<TYPE, SIZE> buffer;
      size_t writeIndex;
      size_t readIndex;
      size_t count;
};
