#include <FastLED.h>
#define NUM_LEDS 30

#ifndef PSTR
#define PSTR // Make Arduino Due happy
#endif

#define MAXSPRITES           2

#define PIR_SENSOR_1_PIN     2
#define PIR_SENSOR_2_PIN     3
#define NEOPIXEL_DATA_PIN    6                // Pin for neopixels

#define INFRARED_SENSOR_TIMEOUT_IN_MS   500

// ...TO HERE.

// Array representing the strip.
CRGB leds[NUM_LEDS];

// Function prototypes.
void resetStrip(void);
void debug(void);

class InputDevice {
  public:
    virtual bool IsActuated() = 0;
    uint32_t lastPollTime;
    
protected:
    int _pinNumber;
};

class InfraredSensor : public InputDevice {
public:
    InfraredSensor(int pinNumber) {
        this->_pinNumber = pinNumber;
    }

    bool IsActuated() {
        // Put sensor read code here. Return true if triggered, false otherwise.
        if (millis() - this->lastPollTime < INFRARED_SENSOR_TIMEOUT_IN_MS) {
            return false;
        }

        // TODO Should this be analogRead?
        if (digitalRead(this->_pinNumber) == HIGH) {
            this->lastPollTime = millis();
            return true;
        }        

        return false;
    }
};


class Sprite {
  public:
    Sprite() {
      lastUpdateTime = 0;
      done = false;
    }

    virtual ~Sprite() {
    }

    virtual bool Update() = 0;

    boolean UpdateNow() {
      if (millis() - lastUpdateTime >= 80) {
        lastUpdateTime = millis();
        return true;
      } else {
        return false;
      }
    }

  protected:
    uint32_t lastUpdateTime;
    boolean done;
};

class SpriteVector {
    private:
        Sprite **sprites;
        int maxCapacity;
        int count;

    public:
        SpriteVector(int maxCap) {
            this->count = 0;
            this->maxCapacity = maxCap;
            sprites = new Sprite*[maxCap];

            for (int i = 0; i < maxCap; i++) {
                sprites[i] = NULL;
            }
        }

        ~SpriteVector() {
        }

        Sprite *Get(int i) {
            if (i < this->count) {
                return sprites[i];
            } else {
                return NULL;
            }

        }

        boolean Add(Sprite *sprite) {
            if (count >= this->maxCapacity) {
                return false;
            }

            sprites[count] = sprite;
            ++count;

            return true;
        }

        int Count() {
            return this->count;
        }

        boolean RemoveAt(int i) {
            Sprite *ptr = sprites[i];
            sprites[i] = NULL;
            delete ptr;

            for (int j = i + 1; j < count; j++) {
                sprites[j - 1] = sprites[j];
                sprites[j] = NULL;
            }

            --count;

            return true;
        }
};

class TestPatternFastLEDSprite : public Sprite {
  private:
    long currentPixel;
    CRGB pixelColor;

  public:
    TestPatternFastLEDSprite() : Sprite() {
      currentPixel = 0;
      pixelColor = CRGB::White;  // White
    }

    TestPatternFastLEDSprite(int startPixel, CRGB color) : Sprite() {
      currentPixel = startPixel;
      pixelColor = color;
    }

    ~TestPatternFastLEDSprite() {
    }

    bool Update() {
        if (this->UpdateNow()) {
            leds[currentPixel % NUM_LEDS] = CRGB::White;
            leds[(currentPixel - 1) % NUM_LEDS] = CRGB::Black;

            ++currentPixel;

            return true;
        } else {
            currentPixel = 0;
        }

        return false;
    }
};


class W4V1ScannerSprite : public Sprite {
  private:
    int currentPixel;
    bool scanning;
    bool hasScanned;
    int scannerCount;

    CRGB regularFlow[4] = { 0x400000, 0x800000, 0xc00000, 0xff0000 };

    const CRGB red0 = 0x000000;
    const CRGB red4 = 0x400000;
    const CRGB red8 = 0x800000;
    const CRGB redC = 0xC00000;
    const CRGB redF = 0xFF0000;

    CRGB debris[78] = { red0, red0, red0, red0, red0, red0, red0, redF, redC, red8, redC, red0, red0,    // frame 10
                        red0, red0, red0, red0, red0, redF, redC, red8, red4, red0, red4, red8, red0,    // frame 11
                        red0, red0, red0, redF, redC, red8, red4, red0, red0, red0, red0, red0, red4,    //       12
                        red0, red0, redC, red8, redC, redF, red0, red0, red0, red0, red0, red0, red0,    //       13
                        red0, red8, red4, red0, red4, red8, redC, redF, red0, red0, red0, red0, red0,    //       14
                        red4, red0, red0, red0, red0, red0, red4, red8, redC, redF, red0, red0, red0     //       15
                      };


  public:
    W4V1ScannerSprite() : Sprite() {
      this->currentPixel = 0;
      this->scanning = false;
      this->hasScanned = false;
      this->scannerCount = 0;

/*
      this->regularFlow2[0] = redF;
      this->regularFlow2[1] = red0;
      this->regularFlow2[2] = red4;
      this->regularFlow2[3] = red8;
      this->regularFlow2[4] = redC;
      this->regularFlow2[5] = redF;
*/

    }

    ~W4V1ScannerSprite() {
    }

    bool Update() {
      if (this->UpdateNow()) {
        if (this->currentPixel >= NUM_LEDS + 3 || this->currentPixel == 0) {
            currentPixel = 0;
            scannerCount = 0;

            memset(leds, 0x00, NUM_LEDS * sizeof(CRGB));
        }

        if (scanning) {
            if (scannerCount >= 6 * 5) {
                memset(leds + 8, 0x00, 13 * sizeof(CRGB));
                scanning = false;
                currentPixel = 18;
            }
        }

        if (! scanning) {
            if (currentPixel >= 0 && currentPixel <= 3) {
                memcpy(leds, regularFlow + (3 - currentPixel), (currentPixel + 1) * sizeof(CRGB));
            } else if (scannerCount == 0 && (currentPixel == 15 || currentPixel == 16)) {
                memcpy(leds + (currentPixel - 3), regularFlow, 4 * sizeof(CRGB));
                leds[currentPixel - 4] = CRGB::Black;
                leds[currentPixel - 5] = CRGB::Black;
                scanning = true;                
            } else if (this->currentPixel >= 4 && this->currentPixel < NUM_LEDS) {
                memcpy(leds + (currentPixel - 3), regularFlow, 4 * sizeof(CRGB));
                leds[currentPixel - 4] = CRGB::Black;
                leds[currentPixel - 5] = CRGB::Black;
            } else if (currentPixel >= NUM_LEDS) {
                memcpy(leds + (currentPixel - 3), regularFlow, (33 - currentPixel) * sizeof(CRGB));
                leds[currentPixel - 4] = CRGB::Black;
                leds[currentPixel - 5] = CRGB::Black;
            }

            currentPixel += 2;
        } else {
            memcpy(leds + 8, debris + (13 * (scannerCount % 6)), 13 * sizeof(CRGB));
            ++scannerCount;
        }

        return true;
      } else {
        return false;
      }
    }
};

class W1V1Sprite : public Sprite {
  private:
    int currentPixel;

  public:
    W1V1Sprite() {
        this->currentPixel = 0;
    }

    ~W1V1Sprite() {
    }

    bool Update() {
      if (this->UpdateNow()) {
        currentPixel = (currentPixel + 1) % NUM_LEDS;
        leds[currentPixel] = 0x0000ff;
        leds[currentPixel - 1] = 0x000000;

        return true;
      }

      return false;
    }
};

class TestingScannerSprite : public Sprite {
  private:
    int currentPixel;
    CRGB pattern[6] = { 0x000000, 0x003300, 0x006600, 0x009900, 0x00cc00, 0x00ff00 };

  public:
    TestingScannerSprite() {
      this->currentPixel = 5;
    }

    ~TestingScannerSprite() {
    }

    bool Update() {
      if (this->UpdateNow()) {
        ++currentPixel;

        if (currentPixel >= 30) {
          currentPixel = 0;
        }

        memcpy(leds + currentPixel, &pattern, 6 * sizeof(CRGB));

        return true;
      }

      return false;
    }
};


class SpriteManager {
  private:
    boolean updatedSomething = false;
    SpriteVector* spriteVector;
    // TestPatternFastLEDSprite* sprite;

  public:
    SpriteManager() {
        spriteVector = new SpriteVector(MAXSPRITES);
    }

    ~SpriteManager() {
       // Don't bother. Should never be called.
    }

    int SpriteCount() {
      return spriteVector->Count();
      // return (sprite != NULL) ? 1 : 0;
    }

    void Update() {
        for (int i = 0; i < this->SpriteCount(); i++) {
            updatedSomething = spriteVector->Get(i)->Update();
        }
    
        if (updatedSomething) {
            FastLED.show();
        }
    
        updatedSomething = false;

        // sprite->Update();
    }

    // Add it to the first free spot we see.
    bool Add(Sprite *newSprite) {
      bool x = spriteVector->Add(newSprite);
      return x;

      // sprite = newSprite;
      // return true;
    }

    /*
        void Clean() {
            for (int i = 0; i < this->SpriteCount(); i++) {
                if (spriteVector->Get(i)->IsDone()) {
                    spriteVector->RemoveAt(i);
                }
            }
        } */
};

InfraredSensor *sensor1;
InfraredSensor *sensor2;
SpriteManager *spriteManager;


// W4V1ScannerSprite* sprite;
// W1V1Sprite* sprite;

void setup() {
    randomSeed(analogRead(0));
   
    spriteManager = new SpriteManager();

    spriteManager->Add(new W1V1Sprite());
    
    sensor1 = new InfraredSensor(PIR_SENSOR_1_PIN);
    sensor2 = new InfraredSensor(PIR_SENSOR_2_PIN);
  
    resetStrip();

    // sprite = new W4V1ScannerSprite();
    // sprite = new W1V1Sprite();
}

int currentPixel = 0;

void loop() {
    if (random(0, 2000000) == 3000) {
        spriteManager->Add(new W1V1Sprite());
        leds[0] = leds[1] = leds[2] = 0xff0000;
    }
  
    if (sensor1->IsActuated()) {
        // TODO Add sprite.
    }

    if (sensor2->IsActuated()) {
        // TODO Add sprite.
    }
  
    spriteManager->Update();
}


/* ---Utility functions --- */

void resetStrip() {
    FastLED.addLeds<NEOPIXEL, NEOPIXEL_DATA_PIN>(leds, NUM_LEDS);

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }

    FastLED.show();
}

void debug(int number) {
    fill_solid(leds, number < NUM_LEDS ? number : NUM_LEDS, CRGB::White);
    FastLED.show();
}

