#pragma once

class CONN
{
public:
	CONN();
	void set(bool value);
	void init(int i);
	bool get();

private:
	bool* stat;
};

