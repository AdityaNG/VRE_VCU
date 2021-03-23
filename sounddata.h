// In format { n, f1, d1, f2, d2,..., fn, dn }
// n-number of elements, f-freq, d-delay in ms
int stop_tone[] = { 4,
  250, 500, 250, 500
};

int start_tone[] = { 8,
  250, 250, 200, 250, 240, 250, 300, 500
};

int boot_tone[] = { 8,
  250, 250, 250, 250, 250, 250, 300, 500
};

int playing = 0;
void tone(byte pin, int freq) {
  ledcSetup(0, 2000, 8); // setup beeper
  ledcAttachPin(pin, 0); // attach beeper
  ledcWriteTone(0, freq); // play tone
  playing = pin; // store pin
}
void noTone() {
  tone(playing, 0);
}

void play_tone(int arr[], int BUZZER_PIN) {
  int n = arr[0] + 1;
  for (int i=1; i<n-1; i+=2) {
    tone(BUZZER_PIN, arr[i]);
    delay(arr[i+1]);
  }

  noTone();  
}
