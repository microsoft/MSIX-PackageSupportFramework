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
		//test_begin("RegLegacy Test DeletionMarkerTests SUCCESS HKCU");
		RegQueryValueEx_SUCCESS_HKCU(result);

		//test_begin("RegLegacy Test DeletionMarkerTests SUCCESS HKLM");
		RegQueryValueEx_SUCCESS_HKLM(result);
	}

	/// <summary>
	/// Test for RegQueryValueExW
	/// Deletion Marker Found
	/// </summary>
	/// <param name="result"></param>
	inline void RegQueryValueEx_FILENOTFOUND(int result)
	{
		//test_begin("RegLegacy Test DeletionMarkerTests FILE_NOT_FOUND HKCU");
		RegQueryValueEx_FILENOTFOUND_HKCU(result);

		//test_begin("RegLegacy Test DeletionMarkerTests FILE_NOT_FOUND HKLM");
		RegQueryValueEx_FILENOTFOUND_HKLM(result);
	}

	/// <summary>
	/// Test for Deletion Marker Fixup
	/// </summary>
	/// <param name="result"></param>
	inline void DeletionMarkerTests(int result)
	{
		//test_begin("RegLegacy Test DeletionMarkerTests SUCCESS");
		RegQueryValueEx_SUCCESS(result);


		//test_begin("RegLegacy Test DeletionMarkerTests FILENOTFOUND");
		RegQueryValueEx_FILENOTFOUND(result);
	}
}
