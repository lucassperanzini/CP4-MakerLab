// Can be moved in header file i.e notes.h
#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define D4 294
#define A4 440
#define Ab4 466
#define Gb4 415
#define C5 523
#define D5 587
#define G4 392
#define Cb5 554

const int midi1[36][3] = {
 {D4, 167, 0},
 {D4, 167, 0},
 {A4, 167, 167},
 {D4, 167, 0},
 {D4, 167, 0},
 {Ab4, 167, 167},
 {D4, 167, 0},
 {D4, 167, 0},
 {A4, 167, 167},
 {D4, 167, 0},
 {D4, 167, 0},
 {Gb4, 167, 167},
 {D4, 167, 0},
 {D4, 167, 0},
 {A4, 167, 167},
 {D4, 167, 0},
 {D4, 167, 0},
 {C5, 167, 167},
 {D5, 167, 500},
 {A4, 167, 500},
 {C5, 167, 500},
 {G4, 167, 500},
 {D4, 167, 0},
 {D4, 167, 0},
 {A4, 167, 167},
 {D4, 167, 0},
 {D4, 167, 0},
 {Ab4, 167, 167},
 {D4, 167, 0},
 {D4, 167, 0},
 {C5, 167, 167},
 {D4, 167, 0},
 {D4, 167, 0},
 {Cb5, 167, 167},
 {D5, 167, 1167},
 {D5, 167, 0},
};

void fireRed1(int pin, int led) {
  size_t len = ARRAY_LEN(midi1);

  for (size_t i = 0; i < len; i++) {
    int notePitch = midi1[i][0];
    int noteDuration = midi1[i][1];
    int pauseDuration = midi1[i][2];

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
