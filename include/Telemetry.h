#define TraceLoggingOptionMicrosoftTelemetry() \
    TraceLoggingOptionGroup(0000000000, 00000, 00000, 0000, 0000, 0000, 0000, 0000, 000, 0000, 0000)

#define TelemetryPrivacyDataTag(tag) TraceLoggingUInt64((tag), "PartA_PrivTags")

#define PDT_BrowsingHistory                    0x0000000000000002u
#define PDT_DeviceConnectivityAndConfiguration 0x0000000000000800u
#define PDT_InkingTypingAndSpeechUtterance     0x0000000000020000u
#define PDT_ProductAndServicePerformance       0x0000000001000000u
#define PDT_ProductAndServiceUsage             0x0000000002000000u
#define PDT_SoftwareSetupAndInventory          0x0000000080000000u

#define MICROSOFT_KEYWORD_CRITICAL_DATA 0x0000800000000000 // Bit 47
#define MICROSOFT_KEYWORD_MEASURES      0x0000400000000000 // Bit 46