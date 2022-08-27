#ifndef EDITOR_H_INC
#define EDITOR_H_INC

void setup_editor(const char* _source_file, const char* _target_file, void (*_editor_started)(), void (*_editor_finished)(), void (*_editor_interrupted)(int));
int editor_start(char* speedup_factor, char* threshold);
int editor_process_chain();
int editor_stop();
int cancel_editor();
void editor_cleanup();
int editor_set_as_input(const char* filename);

#endif