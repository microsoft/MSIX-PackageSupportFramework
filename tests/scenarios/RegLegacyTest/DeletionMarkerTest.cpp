#pragma region Include
#include "Helper.cpp"
#pragma endregion

#pragma region Using
using namespace Helper;
#pragma endregion

namespace DeletionMarkerTest
{
	/// <summary>
	/// Test for RegQueryValueExW
	/// Deletion Marker Not Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegQueryValueEx_SUCCESS(int result)
	{
		RegQueryValueEx_SUCCESS_HKCU(result);

		RegQueryValueEx_SUCCESS_HKLM(result);
	}

	/// <summary>
	/// Test for RegQueryValueExW
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegQueryValueEx_FILENOTFOUND(int result)
	{
		RegQueryValueEx_FILENOTFOUND_HKCU(result);

		RegQueryValueEx_FILENOTFOUND_HKLM(result);
	}

	/// <summary>
	/// Test for RegGetValue
	/// Deletion Marker NOT found
	/// </summary>
	/// <param name="result"></param>
	inline void RegGetValue_SUCCESS(int result)
	{
		RegGetValue_SUCCESS_HKCU(result);
	}

	/// <summary>
	/// Test for RegGetValue
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegGetValue_FILENOTFOUND(int result)
	{
		RegGetValue_FILENOTFOUND_HKLM(result);
	}

	/// <summary>
	/// Test for RegQueryMultipleValues
	/// Deletion Marker Not Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegQueryMultipleValues_SUCCESS(int result)
	{
		RegQueryMultipleValues_SUCCESS_HKCU(result);
	}

	/// <summary>
	/// Test for RegQueryMultipleValues
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegQueryMultipleValues_FILENOTFOUND(int result)
	{
		RegQueryMultipleValues_FILENOTFOUND_HKLM(result);
	}

	/// <summary>
	/// Test for RegEnumValue
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegEnumValue_DeletionMarkerFound(int result)
	{
		RegEnumValue_DeletionMarkerFound_HKLM(result);
	}

	/// <summary>
	/// Test for RegOpenKeyEx
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegOpenKeyEx_DeletionMarkerFound(int result)
	{
		RegOpenKeyEx_FILENOTFOUND_HKLM(result);
	}

	/// <summary>
	/// Test for RegOpenKeyTransacted
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegOpenKeyTransacted_DeletionMarkerFound(int result)
	{
		RegOpenKeyTransacted_FILENOTFOUND_HKLM(result);
	}

	/// <summary>
	/// Test for RegEnumKeyEx
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegEnumKeyEx_DeletionMarkerFound(int result)
	{
		RegEnumKeyEx_FILENOTFOUND_HKLM(result);
	}

	/// <summary>
	/// Test for RegQueryInfoKey
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegQueryInfoKey_SUCCESS(int result)
	{
		RegQueryInfoKey_SUCCESS_HKCU(result);
	}

	/// <summary>
	/// Test for Deletion Marker Fixup
	/// </summary>
	/// <param name="result"></param>
	inline void DeletionMarkerTests(int result)
	{
		RegQueryValueEx_SUCCESS(result);

		RegQueryValueEx_FILENOTFOUND(result);

		RegGetValue_SUCCESS(result);

		RegGetValue_FILENOTFOUND(result);

		RegQueryMultipleValues_SUCCESS(result);

		RegQueryMultipleValues_FILENOTFOUND(result);

		RegEnumValue_DeletionMarkerFound(result);

		RegOpenKeyEx_DeletionMarkerFound(result);

		RegOpenKeyTransacted_DeletionMarkerFound(result);

		RegEnumKeyEx_DeletionMarkerFound(result);

		RegQueryInfoKey_SUCCESS(result);
	}
}
