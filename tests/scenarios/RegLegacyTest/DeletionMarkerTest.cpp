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
	/// Test for Deletion Marker Fixup
	/// </summary>
	/// <param name="result"></param>
	inline void DeletionMarkerTests(int result)
	{
		RegQueryValueEx_SUCCESS(result);

		RegQueryValueEx_FILENOTFOUND(result);
	}
}
