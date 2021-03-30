#pragma once

class CONN
{
public:
	CONN();
	void set(bool value);
	void init();
	bool get();

private:
	bool *stat;
};

extern CONN g_conn;
