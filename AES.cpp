#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <windows.h>

using namespace std;

#define xtime(x) ((x << 1) ^ (((x >> 7) & 1) * 0x169))		//xtime 정의
#define Multiply(x,y) (((y & 1) * x) ^ ((y>>1 & 1) * xtime(x)) ^ ((y>>2 & 1) * xtime(xtime(x))) ^ ((y>>3 & 1) * xtime(xtime(xtime(x)))) ^ ((y>>4 & 1) * xtime(xtime(xtime(xtime(x)))))) //Mix column에 사용되는 곱셈 정의
#define Nb 4	// Block 길이
#define Nk 4	// 키 길이
#define Nr 10	// Round 수
int state[4][4] = { 0, };			// 중간 상태를 저장하는 배열
int RoundKey[240] = { 0, };			// Round Key를 저장하는 배열
int c[8] = { 1,0,1,0,1,0,0,0 };		// Sbox XOR 연산에 사용되는 배열
int c1[8] = { 1,1,1,0,0,0,1,1 };	// Inverse Sbox XOR 연산에 사용되는 배열
int b[8] = { 0, };
int b1[8] = { 0, };
unsigned char sbox[256];
unsigned char invsbox[256];
unsigned char GF[256] = { 0, };	// 역원 구할때 사용되는 GF(2^8)의 배열
unsigned char InvGF[256];		// 역원 구할때 사용되는	역 갈루아 필드
int Sbox(int num) {		// 암호화에 사용되는 8 bit의 Sbox를 생성하는 함수
	int m, w;
	int a = 1;
	for (int e = 0; e < 256; e++) {		// 갈루아 필드 생성하는 함수
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
		m = InvGF[(255 - GF[i])];			// 역원
		for (int j = 7; j >= 0; j--) {
			b[j] = m / (1 << j);			// 역원을 2진수로 만들어 b[0]부터 b[7]까지 저장
			m %= (1 << j);
		}
		for (int k = 0; k < 8; k++) {
			b1[k] = b[k] ^ b[(k + 4) % 8] ^ b[(k + 5) % 8] ^ b[(k + 6) % 8] ^ b[(k + 7) % 8] ^ c[k];	// 행렬 연산 후 XOR
			m += b1[k] * (1 << k);			// 2진수에서 다시 복구
		}
		sbox[i] = m;
	}
	sbox[0] = 0x15;							// 0은 역원이 없으므로 따로 지정
	return sbox[num];
}

int InvSbox(int n) {			// 복호화에 사용되는 8 bit의 Inverse Sbox를 생성하는 함수
	int i, j, k, m;

	for (i = 1; i < 256; i++) {
		m = i;
		for (j = 7; j >= 0; j--) {
			b[j] = m / (1 << j);		// 1부터 255까지 각각 2진수로 만들어 b[0]부터 b[7]까지 저장
			m %= (1 << j);
		}
		for (k = 0; k < 8; k++) {
			b1[k] = b[(k + 2) % 8] ^ b[(k + 5) % 8] ^ b[(k + 7) % 8] ^ c1[k];	// 행렬 연산 후 XOR
			m = m + b1[k] * (1 << k);	// 2진수에서 다시 복구
		}
		invsbox[i] = InvGF[(255 - GF[m])];	// 역원
	}
	sbox[0] = 0xf3;							// 0은 역원이 없으므로 따로 지정
	return invsbox[n];
}

int RCon(int num) {	// Rcon 생성 함수
	int Rcon[11];
	for (int i = 0; i < 10; i++) {
		Rcon[i] = (1 << i) % 0x169;
		if (Rcon[i] > 128)
			Rcon[i] = 0x169 - Rcon[i];
	}
	return Rcon[num];
}

// 각 라운드의 키를 추가하기 위한 키 확장 함수
void KeyExpansion(int *key)
{
	int i, j, d = 0;
	unsigned char temp[4], k;
	cout << "ROUND 0: ";
	// 라운드 키에 키 값을 저장
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
			// 워드 단위로 순환 이동 (RotWord)
			{
				k = temp[0];
				temp[0] = temp[1];
				temp[1] = temp[2];
				temp[2] = temp[3];
				temp[3] = k;
			}

			// Sbox를 통해서 치환 (SubWord)
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
		// 각 라운드의 값 보정
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

// 상태 배열과 라운드 키를 XOR 연산하여 라운드 키 값을 추가하는 함수
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
	// 상태 배열의 각 Byte 값을 Sbox로 치환
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

// 복호화 SubBytes 함수
void InvSubBytes()
{
	cout << "SB: ";
	// 상태 배열의 각 Byte 값을 Inverse Sbox로 치환
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

// Byte 치환이 이루어진 값을 bit 단위로 행 순환시키는 함수
void ShiftRows()
{
	unsigned char temp = 0;

	// 첫 번째 행을 기준으로 왼쪽으로 순환
	temp = state[1][0];
	state[1][0] = state[1][1];
	state[1][1] = state[1][2];
	state[1][2] = state[1][3];
	state[1][3] = temp;

	// 두 번째 행을 기준으로 왼쪽으로 순환
	temp = state[2][0];
	state[2][0] = state[2][2];
	state[2][2] = temp;

	temp = state[2][1];
	state[2][1] = state[2][3];
	state[2][3] = temp;

	// 세 번째 행을 기준으로 왼쪽으로 순환
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

// 복호화 ShiftRows 함수
void InvShiftRows()
{
	unsigned char temp;

	// 첫 번째 행을 기준으로 오른쪽으로 순환
	temp = state[1][3];
	state[1][3] = state[1][2];
	state[1][2] = state[1][1];
	state[1][1] = state[1][0];
	state[1][0] = temp;

	// 두 번째 행을 기준으로 오른쪽으로 순환
	temp = state[2][0];
	state[2][0] = state[2][2];
	state[2][2] = temp;

	temp = state[2][1];
	state[2][1] = state[2][3];
	state[2][3] = temp;

	// 셋 번째 행을 기준으로 오른쪽으로 순환
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

// 4 bit 단위로 연산하여 열 혼합시키는 함수
void MixColumns()
{
	unsigned char Tmp, Tm, t;

	for (int i = 0; i < 4; i++)
	{
		t = state[0][i];

		Tmp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i];
		// 행렬 곱셈 연산을 수행하여 Tm에 저장
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

// 복호화 MixColumns 함수
void InvMixColumns()
{
	unsigned char a, b, c, d;
	for (int i = 0; i < 4; i++)
	{

		a = state[0][i];
		b = state[1][i];
		c = state[2][i];
		d = state[3][i];

		// Multiply로 고정된 다항식과 곱셈 연산 후 XOR 연산
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

void Cipher(int * plain, int * cipher)	// 암호화 함수
{
	// 평문을 상태 배열에 저장
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < Nb; j++)
		{
			state[j][i] = plain[4 * i + j];
		}
	}
	cout << endl ;

	cout << "Round 0" << endl;
	AddRoundKey(0); // 첫 번째 라운드 키 추가
	cout << endl;
	for (int round = 1; round < Nr; round++)
	{
		cout << "Round " << round << endl;
		SubBytes();			// Byte 단위 치환
		ShiftRows();		// 행 순환
		MixColumns();		// 열 혼합
		AddRoundKey(round); // 확장한 키와 현재 블록을 XOR 연산
		cout << endl;
	}
	cout << "Round 10" << endl;
	SubBytes();
	ShiftRows();
	AddRoundKey(Nr); // 마지막 키와 현재 블록을 XOR 연산


	// 최종 암호화 상태 배열을 cipher 배열에 저장
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < Nb; j++)
		{
			cipher[4 * i + j] = state[j][i];
		}
	}
}

void InvCipher(int * cipher, int * decrypted){		// 복호화 함수
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < Nb; j++)
		{
			state[j][i] = cipher[4 * i + j];
		}
	}

	cout << "Round 0" <<endl;
	AddRoundKey(Nr);		// 마지막 키와 현재 블록을 XOR 연산
	cout << endl;
	for (int round = Nr - 1; round > 0; round--)
	{
		cout << "Round " << 10 - round <<endl ;
		InvShiftRows();		// 역 행 순환
		InvSubBytes();		// 역 바이트 치환
		AddRoundKey(round);	// 라운드 키 추가
		InvMixColumns();	// 역 열 혼합
		cout << endl;
	}
	cout << "Round 10" << endl;
	InvShiftRows();
	InvSubBytes();
	AddRoundKey(0);			// 첫 번째 키와 현재 블록을 XOR 연산
	cout << endl;
	// 최종 복호화 상태 배열을 plain 배열에 저장
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
	int key[32];		// 키 저장하는 배열
	int plain[20];		// 평문 저장하는 배열
	int decrypted[20];	// 복호문 저장하는 배열
	unsigned char buf[16];
	unsigned char buf1[16];
	// 파일 포인터 선언
	FILE *fp;
	FILE *fp1;
	FILE *fp2;
	FILE *fp3;

	cout << hex;

	cout << "RC : ";
	for (int i = 0; i < 10; i++)
		cout << RCon(i) << " ";
	cout << endl;

	// 이진 파일 읽기
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
	// 평문 배열 출력
	while (fread((void*)buf, 1, sizeof(buf), fp1)) {
		cout << "PLAIN : ";
		for (i = 0; i < 16; i++) {
			plain[i] = buf[i];
			cout << plain[i] << " ";
		}
		cout << endl;
	}
	// 키 배열 출력
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

	int cipher[128] = { 0, };	// 암호문 저장하는 배열
	cout << endl<< endl<<"<------ENCRYPTION-------->" << endl << endl << "KEY EXPANSION" << endl;
	// 키 확장
	KeyExpansion(key);
	Cipher(plain, cipher);		// 암호화
	cout << endl<< "CIPHER : ";
	for (int i = 0; i < 16; i++)
		cout << cipher[i] << " ";
	cout << endl<<endl<<"<------DECRYPTION-------->" << endl << endl;
	
	InvCipher(cipher, decrypted);	// 복호화
	cout << "DECRYPTED : ";
	for (int i = 0; i < 16; i++)
		cout << decrypted[i] << " ";
	cout << endl;
	
	// 암호문과 복호문 이진 파일로 작성
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
	// 암호문 파일 쓰기
	for (i = 0; i < 16; i++) {
		buf1[i] = cipher[i];
	}
	fwrite(&buf1, 1, sizeof(buf1), fp2);
	
	// 복호문 파일 쓰기
	for (i = 0; i < 16; i++) {
		buf1[i] = decrypted[i];
	}
	fwrite(&buf1, 1, sizeof(buf1), fp3);

	fclose(fp2);
	fclose(fp3);

	system("pause");
	return 0;
}