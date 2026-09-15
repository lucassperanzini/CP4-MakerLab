// Can be moved in header file i.e notes.h
#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define G4 392
#define F4 349
#define G3 196
#define B3 247
#define D4 294
#define E4 330
#define Db4 311
#define F3 175
#define E3 165
#define Db3 156
#define D3 147

const int midi2[36][3] = {
 {G4, 136, 273},
 {G4, 136, 0},
 {G4, 136, 0},
 {G4, 136, 0},
 {G4, 136, 273},
 {G4, 136, 0},
 {G4, 136, 0},
 {G4, 136, 0},
 {G4, 136, 273},
 {G4, 136, 273},
 {F4, 136, 136},
 {F4, 136, 136},
 {F4, 136, 136},
 {G3, 136, 409},
 {B3, 136, 136},
 {D4, 136, 1227},
 {F4, 136, 409},
 {E4, 136, 0},
 {Db4, 136, 0},
 {D4, 136, 682},
 {F3, 136, 409},
 {E3, 136, 0},
 {Db3, 136, 0},
 {D3, 136, 545},
 {D3, 136, 136},
 {D3, 136, 136},
 {D3, 136, 136},
 {G3, 136, 409},
 {B3, 136, 136},
 {D4, 136, 1091},
 {F3, 136, 136},
 {F3, 136, 136},
 {F3, 136, 273},
 {G3, 136, 409},
 {B3, 136, 136},
 {G3, 136, 0},
};

void fireRedIntro(int pin, int led) {
  size_t len = ARRAY_LEN(midi2);

  for (size_t i = 0; i < len; i++) {
    int notePitch = midi2[i][0];
    int noteDuration = midi2[i][1];
    int pauseDuration = midi2[i][2];

    // Se a nota não for silêncio (frequência > 0)
    if (notePitch > 0) {
      // Toca a nota por apenas 90% do tempo total para separar dos sons seguintes
      tone(pin, notePitch, noteDuration * 0.9);
       digitalWrite(led, HIGH); 
    }

    // Espera a duração inteira da nota soar
    delay(noteDuration);
    noTone(pin);

     digitalWrite(led, LOW); 

    // Se houver uma pausa extra definida na matriz, aplica aqui
    if (pauseDuration > 0) {
      delay(pauseDuration);
    }
  }
 
}
// Generated using https://github.com/ShivamJoker/MIDI-to-Arduino
