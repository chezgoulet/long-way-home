/* Cutscene stubs for builds without FFmpeg.
 *
 * The port decodes Bink movies through libavcodec (sp/bink_ff.c). Where FFmpeg's development
 * files are unavailable -- on a host whose ffmpeg packages sit behind Ubuntu Pro, for instance --
 * the engine can still be built and played; cinematics are the part that is missing, and they
 * are polish, not core. Each stub states what is lost rather than pretending to play a movie.
 *
 * Bink_IsBinkFile reports 0, so a .bik is treated as "not a movie" and the caller skips it instead
 * of attempting to open one. That is the difference between a cutscene that does not play and a
 * crash where a cutscene should be.
 */
int Bink_IsBinkFile(const char *name)
{
  (void)name;
  return 0;
}

int Bink_Open(int handle, const char *name, int *outW, int *outH)
{
  (void)handle; (void)name; (void)outW; (void)outH;
  return 0;
}

int Bink_Update(int handle, int startMs, int nowMs, int silent, unsigned char **outBuf)
{
  (void)handle; (void)startMs; (void)nowMs; (void)silent; (void)outBuf;
  return 0;
}

void Bink_Restart(int handle) { (void)handle; }

void Bink_Stop(int handle) { (void)handle; }
