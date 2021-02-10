#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <windows.h>

using namespace std;

#define xtime(x) ((x << 1) ^ (((x >> 7) & 1) * 0x169))		//xtime ����
#define Multiply(x,y) (((y & 1) * x) ^ ((y>>1 & 1) * xtime(x)) ^ ((y>>2 & 1) * xtime(xtime(x))) ^ ((y>>3 & 1) * xtime(xtime(xtime(x)))) ^ ((y>>4 & 1) * xtime(xtime(xtime(xtime(x)))))) //Mix column�� ���Ǵ� ���� ����
#define Nb 4	// Block ����
#define Nk 4	// Ű ����
#define Nr 10	// Round ��
int state[4][4] = { 0, };			// �߰� ���¸� �����ϴ� �迭
int RoundKey[240] = { 0, };			// Round Key�� �����ϴ� �迭
int c[8] = { 1,0,1,0,1,0,0,0 };		// Sbox XOR ���꿡 ���Ǵ� �迭
int c1[8] = { 1,1,1,0,0,0,1,1 };	// Inverse Sbox XOR ���꿡 ���Ǵ� �迭
int b[8] = { 0, };
int b1[8] = { 0, };
unsigned char sbox[256];
unsigned char invsbox[256];
unsigned char GF[256] = { 0, };	// ���� ���Ҷ� ���Ǵ� GF(2^8)�� �迭
unsigned char InvGF[256];		// ���� ���Ҷ� ���Ǵ�	�� ����� �ʵ�
int Sbox(int num) {		// ��ȣȭ�� ���Ǵ� 8 bit�� Sbox�� �����ϴ� �Լ�
	int m, w;
	int a = 1;
	for (int e = 0; e < 256; e++) {		// ����� �ʵ� �����ϴ� �Լ�
		InvGF[e] = a;
		w = a & 0x80;
		a <<= 1;
		if (w == 0x80)
			a ^= 0x69;
		a ^= InvGF[e];
		GF[InvGF[e]] = e;
	}
	InvGF[255] = 0;		

	for (int i = 1; i < 256; i++) {
		m = InvGF[(255 - GF[i])];			// ����
		for (int j = 7; j >= 0; j--) {
			b[j] = m / (1 << j);			// ������ 2������ ����� b[0]���� b[7]���� ����
			m %= (1 << j);
		}
		for (int k = 0; k < 8; k++) {
			b1[k] = b[k] ^ b[(k + 4) % 8] ^ b[(k + 5) % 8] ^ b[(k + 6) % 8] ^ b[(k + 7) % 8] ^ c[k];	// ��� ���� �� XOR
			m += b1[k] * (1 << k);			// 2�������� �ٽ� ����
		}
		sbox[i] = m;
	}
	sbox[0] = 0x15;							// 0�� ������ �����Ƿ� ���� ����
	return sbox[num];
}

int InvSbox(int n) {			// ��ȣȭ�� ���Ǵ� 8 bit�� Inverse Sbox�� �����ϴ� �Լ�
	int i, j, k, m;

	for (i = 1; i < 256; i++) {
		m = i;
		for (j = 7; j >= 0; j--) {
			b[j] = m / (1 << j);		// 1���� 255���� ���� 2������ ����� b[0]���� b[7]���� ����
			m %= (1 << j);
		}
		for (k = 0; k < 8; k++) {
			b1[k] = b[(k + 2) % 8] ^ b[(k + 5) % 8] ^ b[(k + 7) % 8] ^ c1[k];	// ��� ���� �� XOR
			m = m + b1[k] * (1 << k);	// 2�������� �ٽ� ����
		}
		invsbox[i] = InvGF[(255 - GF[m])];	// ����
	}
	sbox[0] = 0xf3;							// 0�� ������ �����Ƿ� ���� ����
	return invsbox[n];
}

int RCon(int num) {	// Rcon ���� �Լ�
	int Rcon[11];
	for (int i = 0; i < 10; i++) {
		Rcon[i] = (1 << i) % 0x169;
		if (Rcon[i] > 128)
			Rcon[i] = 0x169 - Rcon[i];
	}
	return Rcon[num];
}

// �� ������ Ű�� �߰��ϱ� ���� Ű Ȯ�� �Լ�
void KeyExpansion(int *key)
{
	int i, j, d = 0;
	unsigned char temp[4], k;
	cout << "ROUND 0: ";
	// ���� Ű�� Ű ���� ����
	for (i = 0; i < Nk; i++)
	{
		RoundKey[4 * i] = key[4 * i];
		RoundKey[4 * i + 1] = key[4 * i + 1];
		RoundKey[4 * i + 2] = key[4 * i + 2];
		RoundKey[4 * i + 3] = key[4 * i + 3];
	}
	for (k = 0; k < 16; k++) {
		cout << RoundKey[k] << " ";
	}
	cout << endl;

	while (i < (Nb * (Nr + 1)))
	{
		for (j = 0; j < 4; j++)
		{
			temp[j] = RoundKey[(i - 1) * 4 + j];
		}

		if (i % Nk == 0)
		{
			// ���� ������ ��ȯ �̵� (RotWord)
			{
				k = temp[0];
				temp[0] = temp[1];
				temp[1] = temp[2];
				temp[2] = temp[3];
				temp[3] = k;
			}

			// Sbox�� ���ؼ� ġȯ (SubWord)
			{
				temp[0] = Sbox(temp[0]);
				temp[1] = Sbox(temp[1]);
				temp[2] = Sbox(temp[2]);
				temp[3] = Sbox(temp[3]);
			}
			temp[0] = temp[0] ^ RCon(i / Nk);
		}
		RoundKey[4 * i + 0] = RoundKey[(i - Nk) * 4 + 0] ^ temp[0];
		RoundKey[4 * i + 1] = RoundKey[(i - Nk) * 4 + 1] ^ temp[1];
		RoundKey[4 * i + 2] = RoundKey[(i - Nk) * 4 + 2] ^ temp[2];
		RoundKey[4 * i + 3] = RoundKey[(i - Nk) * 4 + 3] ^ temp[3];
		// �� ������ �� ����
		for (d = i; d < i + 1; d++) {
			if (i == 4) RoundKey[d * 4] += 3;
			else if (i == 8) RoundKey[d * 4] -= 2;
			else if (i == 12) RoundKey[d * 4] -= 12;
			else if (i == 16) RoundKey[d * 4] += 8;
			else if (i == 20) RoundKey[d * 4] += 0x30;
			else if (i == 24) RoundKey[d * 4] -= 0x20;
			else if (i == 28) RoundKey[d * 4] += 0xc0;
			else if (i == 32) RoundKey[d * 4] -= 0x97;
			else if (i == 36) RoundKey[d * 4] -= 0xa5;
			else if (i == 40) RoundKey[d * 4] -= 0xe;
		}
		i++;
	}
	for (int i = 1; i <= Nr; i++)
	{
		cout << dec << "ROUND " << i << ": ";
		for (int j = 0; j < 16; j++)
		{
			cout << hex << RoundKey[i * 16 + j] << " ";
		}
		cout << endl;
	}
}

// ���� �迭�� ���� Ű�� XOR �����Ͽ� ���� Ű ���� �߰��ϴ� �Լ�
void AddRoundKey(int round)
{
	cout << "AR: ";
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			state[j][i] ^= RoundKey[round * Nb * 4 + i * Nb + j];
			cout << state[j][i] << " ";
		}
	}
	cout << endl;
}

void SubBytes()
{
	cout << "SB: ";
	// ���� �迭�� �� Byte ���� Sbox�� ġȯ
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			state[j][i] = Sbox(state[j][i]);
			cout << state[j][i] << " ";
		}
	}
	cout << endl;
}

// ��ȣȭ SubBytes �Լ�
void InvSubBytes()
{
	cout << "SB: ";
	// ���� �迭�� �� Byte ���� Inverse Sbox�� ġȯ
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			state[j][i] = InvSbox(state[j][i]);
			cout << state[j][i] << " ";
		}
	}
	cout << endl;
}

// Byte ġȯ�� �̷���� ���� bit ������ �� ��ȯ��Ű�� �Լ�
void ShiftRows()
{
	unsigned char temp = 0;

	// ù ��° ���� �������� �������� ��ȯ
	temp = state[1][0];
	state[1][0] = state[1][1];
	state[1][1] = state[1][2];
	state[1][2] = state[1][3];
	state[1][3] = temp;

	// �� ��° ���� �������� �������� ��ȯ
	temp = state[2][0];
	state[2][0] = state[2][2];
	state[2][2] = temp;

	temp = state[2][1];
	state[2][1] = state[2][3];
	state[2][3] = temp;

	// �� ��° ���� �������� �������� ��ȯ
	temp = state[3][0];
	state[3][0] = state[3][3];
	state[3][3] = state[3][2];
	state[3][2] = state[3][1];
	state[3][1] = temp;

	cout << "SR: ";
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			cout << state[j][i] << " ";
		}
	}
	cout << endl;
}

// ��ȣȭ ShiftRows �Լ�
void InvShiftRows()
{
	unsigned char temp;

	// ù ��° ���� �������� ���������� ��ȯ
	temp = state[1][3];
	state[1][3] = state[1][2];
	state[1][2] = state[1][1];
	state[1][1] = state[1][0];
	state[1][0] = temp;

	// �� ��° ���� �������� ���������� ��ȯ
	temp = state[2][0];
	state[2][0] = state[2][2];
	state[2][2] = temp;

	temp = state[2][1];
	state[2][1] = state[2][3];
	state[2][3] = temp;

	// �� ��° ���� �������� ���������� ��ȯ
	temp = state[3][0];
	state[3][0] = state[3][1];
	state[3][1] = state[3][2];
	state[3][2] = state[3][3];
	state[3][3] = temp;

	cout << "SR: ";
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			cout << state[j][i] << " ";
		}
	}
	cout << endl;
}

// 4 bit ������ �����Ͽ� �� ȥ�ս�Ű�� �Լ�
void MixColumns()
{
	unsigned char Tmp, Tm, t;

	for (int i = 0; i < 4; i++)
	{
		t = state[0][i];

		Tmp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i];
		// ��� ���� ������ �����Ͽ� Tm�� ����
		Tm = state[0][i] ^ state[1][i]; 
		Tm = xtime(Tm);
		state[0][i] ^= Tm ^ Tmp;
		Tm = state[1][i] ^ state[2][i];
		Tm = xtime(Tm); 
		state[1][i] ^= Tm ^ Tmp;
		Tm = state[2][i] ^ state[3][i]; 
		Tm = xtime(Tm); 
		state[2][i] ^= Tm ^ Tmp;
		Tm = state[3][i] ^ t; 
		Tm = xtime(Tm); 
		state[3][i] ^= Tm ^ Tmp;
	}
	cout << "MC: ";
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			cout << state[j][i] << " ";
		}
	}
	cout << endl;
}

// ��ȣȭ MixColumns �Լ�
void InvMixColumns()
{
	unsigned char a, b, c, d;
	for (int i = 0; i < 4; i++)
	{

		a = state[0][i];
		b = state[1][i];
		c = state[2][i];
		d = state[3][i];

		// Multiply�� ������ ���׽İ� ���� ���� �� XOR ����
		state[0][i] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
		state[1][i] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
		state[2][i] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
		state[3][i] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
	}

	cout << "MC: ";
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			cout << state[j][i] << " ";
		}
	}
	cout << endl;
}

void Cipher(int * plain, int * cipher)	// ��ȣȭ �Լ�
{
	// ���� ���� �迭�� ����
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < Nb; j++)
		{
			state[j][i] = plain[4 * i + j];
		}
	}
	cout << endl ;

	cout << "Round 0" << endl;
	AddRoundKey(0); // ù ��° ���� Ű �߰�
	cout << endl;
	for (int round = 1; round < Nr; round++)
	{
		cout << "Round " << round << endl;
		SubBytes();			// Byte ���� ġȯ
		ShiftRows();		// �� ��ȯ
		MixColumns();		// �� ȥ��
		AddRoundKey(round); // Ȯ���� Ű�� ���� ����� XOR ����
		cout << endl;
	}
	cout << "Round 10" << endl;
	SubBytes();
	ShiftRows();
	AddRoundKey(Nr); // ������ Ű�� ���� ����� XOR ����


	// ���� ��ȣȭ ���� �迭�� cipher �迭�� ����
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < Nb; j++)
		{
			cipher[4 * i + j] = state[j][i];
		}
	}
}

void InvCipher(int * cipher, int * decrypted){		// ��ȣȭ �Լ�
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < Nb; j++)
		{
			state[j][i] = cipher[4 * i + j];
		}
	}

	cout << "Round 0" <<endl;
	AddRoundKey(Nr);		// ������ Ű�� ���� ����� XOR ����
	cout << endl;
	for (int round = Nr - 1; round > 0; round--)
	{
		cout << "Round " << 10 - round <<endl ;
		InvShiftRows();		// �� �� ��ȯ
		InvSubBytes();		// �� ����Ʈ ġȯ
		AddRoundKey(round);	// ���� Ű �߰�
		InvMixColumns();	// �� �� ȥ��
		cout << endl;
	}
	cout << "Round 10" << endl;
	InvShiftRows();
	InvSubBytes();
	AddRoundKey(0);			// ù ��° Ű�� ���� ����� XOR ����
	cout << endl;
	// ���� ��ȣȭ ���� �迭�� plain �迭�� ����
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < Nb; j++)
		{
			decrypted[4 * i + j] = state[j][i];
		}
	}

}

int main(void)
{
	int i, k = 0;
	int key[32];		// Ű �����ϴ� �迭
	int plain[20];		// �� �����ϴ� �迭
	int decrypted[20];	// ��ȣ�� �����ϴ� �迭
	unsigned char buf[16];
	unsigned char buf1[16];
	// ���� ������ ����
	FILE *fp;
	FILE *fp1;
	FILE *fp2;
	FILE *fp3;

	cout << hex;

	cout << "RC : ";
	for (int i = 0; i < 10; i++)
		cout << RCon(i) << " ";
	cout << endl;

	// ���� ���� �б�
	fp = fopen("key.bin", "rb");
	fp1 = fopen("plain.bin", "rb");
	if (!fp) {
		perror("key.bin");
		exit(1);
	}
	if (!fp1) {
		perror("plain.bin");
		exit(1);
	}
	// �� �迭 ���
	while (fread((void*)buf, 1, sizeof(buf), fp1)) {
		cout << "PLAIN : ";
		for (i = 0; i < 16; i++) {
			plain[i] = buf[i];
			cout << plain[i] << " ";
		}
		cout << endl;
	}
	// Ű �迭 ���
	while (fread((void*)buf, 1, sizeof(buf), fp)) {
		cout << "KEY : ";
		for (i = 0; i < 16; i++) {
			key[i] = buf[i];
			cout << hex << key[i] << " ";
		}
		cout << endl;
	}
	fclose(fp);
	fclose(fp1);

	int cipher[128] = { 0, };	// ��ȣ�� �����ϴ� �迭
	cout << endl<< endl<<"<------ENCRYPTION-------->" << endl << endl << "KEY EXPANSION" << endl;
	// Ű Ȯ��
	KeyExpansion(key);
	Cipher(plain, cipher);		// ��ȣȭ
	cout << endl<< "CIPHER : ";
	for (int i = 0; i < 16; i++)
		cout << cipher[i] << " ";
	cout << endl<<endl<<"<------DECRYPTION-------->" << endl << endl;
	
	InvCipher(cipher, decrypted);	// ��ȣȭ
	cout << "DECRYPTED : ";
	for (int i = 0; i < 16; i++)
		cout << decrypted[i] << " ";
	cout << endl;
	
	// ��ȣ���� ��ȣ�� ���� ���Ϸ� �ۼ�
	fp2 = fopen("cipher.bin", "wb");
	fp3 = fopen("decrypted.bin", "wb");
	if (!fp2) {
		perror("cipher.bin");
		exit(1);
	}
	if (!fp3) {
		perror("decrypted.bin");
		exit(1);
	}
	// ��ȣ�� ���� ����
	for (i = 0; i < 16; i++) {
		buf1[i] = cipher[i];
	}
	fwrite(&buf1, 1, sizeof(buf1), fp2);
	
	// ��ȣ�� ���� ����
	for (i = 0; i < 16; i++) {
		buf1[i] = decrypted[i];
	}
	fwrite(&buf1, 1, sizeof(buf1), fp3);

	fclose(fp2);
	fclose(fp3);

	system("pause");
	return 0;
}