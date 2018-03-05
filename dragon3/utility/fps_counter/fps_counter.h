#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

class Fps_counter
{
public:
	Fps_counter() : fps(0.0f), lastTime(0.0f), frames(0), time(0.0f)
	{}
	~Fps_counter() {}

	void Update(void);
	float GetFps(void) { return fps; }

protected:
	float fps;
	float lastTime;
	int frames;
	float time;
};

#endif
