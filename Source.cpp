#include <clocale>
#include <cstdio>
#include <cstdint>

bool check_is_32();
bool check_cpuid_supported();
int32_t get_max_cpuid_value();
bool get_vendor(char* _pVendorString);
int32_t get_signature();
int32_t get_extended_info();
bool get_feature_flags(int32_t * _flags1, int32_t * _flags2);
/*-------------------------------------------------------------------*/
int main()
{
	setlocale(LC_CTYPE, "rus");
	int32_t flags1, flags2;

	bool is_32 = check_is_32();
	bool is_cpuid_command_supported = check_cpuid_supported();
	int32_t max_cpuid_value = get_max_cpuid_value();
	int32_t signature = get_signature();
	int32_t additional_info = get_extended_info();
	bool feature_flags_got = get_feature_flags(&flags1, &flags2);

	
	
	//	1. Используя регистр флагов, необходимо убедиться в наличии 32-
	//	разрядного процессора в системе.
	if (is_32)
		printf("Процессор поддерживает 32-х битные команды\n");
	else
		printf("This processor is 16-bit\n");


	//	2.1. Убедиться в поддержке команды CPUID (посредством бита ID регистра EFLAGS)...
	//	2.2. ...и определить максимальное значение параметра ее (CPUID) вызова.
	if (is_cpuid_command_supported) {
		printf("Процессор поддерживает команды CPUID\n");
		printf("Max CPUID input value: %#x\n", max_cpuid_value); // printout in hex (like 0x00ff00)
	}
	else
	{
		printf("There is no CPUID support for this processor\n");
		return 0;
	}

	//	3. Получить строку идентификации производителя процессора и	сохранить ее в памяти.
	const size_t VENDOR_STRING_SIZE = 13; // 12 characters + null-terminator

	char vendor[VENDOR_STRING_SIZE];
	vendor[VENDOR_STRING_SIZE - 1] = '\0';

	if (get_vendor(vendor)) 
		printf("Производитель процессора: %s\n", vendor);
	else
		printf("Unable to get CPU vendor");
	
	//	4.1. Получить сигнатуру процессора и определить его модель, семейство и т.п.

	if (signature >= 0) 
		printf("Сигнатура процессора: %#x\n", signature);
	else 
		printf("Failed to get signature\n");
	
	//	4.2. Выполнить анализ дополнительной информации о процессоре.
	if (additional_info >= 0) 
		printf("Дополнительная информация о процессоре: %#x\n", additional_info);
	else 
		printf("Failed to get extended info\n");

	//	5. Получить флаги свойств. Составить список поддерживаемых процессором свойств.
	if (feature_flags_got) 
		printf("Флаги свойств процессора: %#x %#x\n", flags1, flags2);
	else 
		printf("Failed to get feature flags\n");

	return 0;
}
/*-------------------------------------------------------------------*/




//	1
bool check_is_32() {
	//	1. Используя регистр флагов, необходимо убедиться в наличии 32-
	//	разрядного процессора в системе.
	bool is_32x;
	_asm {
		mov is_32x, 0		; сброс результата
		pushf				; (Сохранить исходное состояние регистра флагов)
		pushf				; Скопировать регистр флагов...
		pop ax				; ...в регистр AX
		xor ah, 11110000b	; Поменять значение старших 4 битов
		push ax				; Скопировать регистр AX
		popf				; ...в регистр флагов
		pushf				; Скопировать регистр флагов...
		pop bx				; ...в регистр BX
		popf				; (Восстановить исходное состояние регистра флагов)
		xor ah, bh			; AH = 0 (биты в регистре флагов не поменялись) → 808x - 80286, иначе 80386 +
		mov is_32x, ah		; save results to boolean value
	}
	return is_32x;
}

//	2.1
bool check_cpuid_supported() {
	//	2.1. Убедиться в поддержке команды CPUID (посредством бита ID регистра EFLAGS)...
	if (check_is_32() == false)
		return false;
	bool is_cpuid_command_supported;
	_asm {
		mov is_cpuid_command_supported, 0		; сбрасываем результат
		pushfd						; Скопировать оригинальное состояние регистра флагов
		pushfd						; Скопировать регистр флагов...
		pop eax						; ...в регистр EAX
		or eax, 200000h				; установить 21 - й бит(флаг ID) в 1
		push eax					; Скопировать регистр EAX...
		popfd						; ...в регистр флагов
		pushf						; Скопировать регистр флагов...
		pop bx						; ...в регистр EBX
		popfd						; (Восстановить исходное состояние регистра флагов)
		xor eax, ebx				; EAH = 0 (биты в регистре флагов не поменялись) → CPUID не подерживается, иначе - поддерживается
		jz exit						; если флаг ID = 0 → CPUID не поддерживается, можно выходить
		mov is_cpuid_command_supported, 1		; иначе - поддержка присутствует
		exit :
	}
	return is_cpuid_command_supported;
}

//	2.2
int32_t get_max_cpuid_value() {
	//	2.2. ...и определить максимальное значение параметра ее (CPUID) вызова.
	if (check_cpuid_supported() == false)
		return -1;
	int32_t max_cpuid_value;
	_asm {
		mov EAX, 00h					; установка входного параметра
		cpuid							; получение выходных параметров, соответствующих входному
		mov max_cpuid_value, eax		; сохранение максимального входного параметра
	}
	return max_cpuid_value;
}

//	3
bool get_vendor(char* _pVendorString) {
	//	3. Получить строку идентификации производителя процессора и	сохранить ее в памяти.
	if (check_cpuid_supported() == false)
		return false;
	_asm {
		mov EAX, 00h				; установка входного параметра
		cpuid						; получение выходных параметров, соответствующих входному
		mov edi, _pVendorString		;
		mov[edi], EBX				; копируем первые 4 символа
		mov[edi + 4], EDX			; копируем вторые 4 символа
		mov[edi + 8], ECX			; копируем последние 4 символа
	}
	return true;
}

//	4.1
int32_t get_signature() {
	//	4.1. Получить сигнатуру процессора и определить его модель, семейство и т.п.
	if (check_cpuid_supported() == false)
		return -1;
	int32_t signature;
	_asm {
		mov EAX, 01h			; установка входного параметра
		cpuid					; получение выходных параметров, соответствующих входному
		mov signature, eax		; сохранение сигнатуры
	}
	return signature;
}

//	4.
int32_t get_extended_info() {
	//	4.2. Выполнить анализ дополнительной информации о процессоре.
	if (check_cpuid_supported() == false)
		return -1;
	int32_t additional_info;
	_asm {
		mov EAX, 01h				; установка входного параметра
		cpuid						; получение выходных параметров, соответствующих входному
		mov additional_info, EBX		; сохранение расширенной информации
	}
	return additional_info;
}

//	5
bool get_feature_flags(int32_t * _flags1, int32_t * _flags2) {
	//	5. Получить флаги свойств. Составить список поддерживаемых процессором свойств.
	if (check_cpuid_supported() == false)
		return false;
	_asm {
		mov EAX, 01h			; установка входного параметра
		cpuid					; получение выходных параметров, соответствующих входному
		mov esi, _flags1		;
		mov edi, _flags2		;
		mov[esi], EDX			; сохранение флагов свойств из EDX
		mov[edi], ECX			; сохранение флагов свойств из ECX
	}
	return true;
}