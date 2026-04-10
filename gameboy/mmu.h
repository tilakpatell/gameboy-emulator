

class MMU {
private:


public:
	MMU();
	~MMU();
	u8 read(u16 address);
	void write(u16 address, u8 value);
};