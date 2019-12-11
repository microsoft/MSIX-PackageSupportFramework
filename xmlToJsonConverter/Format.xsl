<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:template match="configuration">
{
    "applications" : [
    <xsl:for-each select="applications/application">
        {
            "id": "<xsl:value-of select="id"/>",
            "executable": "<xsl:value-of select="executable"/>"
            <xsl:if test="arguments">
            , "arguments": "<xsl:value-of select="arguments"/>"
            </xsl:if>
            <xsl:if test="workingDirectory">
            , "workingDirectory": "<xsl:value-of select="workingDirectory"/>"
            </xsl:if>
            <xsl:if test="stopOnScriptError">
            ,"stopOnScriptError": "<xsl:value-of select="stopOnScriptError"/>"
            </xsl:if>
            <xsl:if test="monitor">
            , "monitor" :
            {
                "executable": "<xsl:value-of select="monitor/executable"/>"
                , "arguments": "<xsl:value-of select="monitor/arguments"/>"
                <xsl:if test="monitor/asadmin">
                    , "asadmin": <xsl:value-of select="monitor/asadmin"/>"
                </xsl:if>
                <xsl:if test="wait">
                    ,"wait": "<xsl:value-of select="monitor/wait"/>"
                </xsl:if>
            }
            </xsl:if>
            <xsl:if test="startScript">
                <xsl:variable name="startScriptHeader" select="startScript" />
                ,"startScript":
                {
                     "scriptPath": "<xsl:value-of select="$startScriptHeader/scriptPath"/>"
                    , "scriptArguments": "<xsl:value-of select="$startScriptHeader/scriptArguments"/>"
                    
                    <xsl:if test="$startScriptHeader/runInVirtualEnvironment">
                    , "runInVirtualEnvironment": <xsl:value-of select="$startScriptHeader/runInVirtualEnvironment"/>
                    </xsl:if>
                    
                    <xsl:if test="$startScriptHeader/runOnce">
                    , "runOnce": <xsl:value-of select="$startScriptHeader/runOnce"/>
                    </xsl:if>
                    
                    <xsl:if test="$startScriptHeader/waitForScriptToFinish">
                    , "waitForScriptToFinish": <xsl:value-of select="$startScriptHeader/waitForScriptToFinish"/>
                    </xsl:if>
                    
                    <xsl:if test="$startScriptHeader/timeout">
                    , "timeout": "<xsl:value-of select="$startScriptHeader/timeout"/>"
                    </xsl:if>
                }
            </xsl:if>
            <xsl:if test="endScript">
            ,
                <xsl:variable name="endScriptHeader" select="endScript" />
                "endScript":
                {
                     "scriptPath": "<xsl:value-of select="$endScriptHeader/scriptPath"/>"
                    , "scriptArguments": "<xsl:value-of select="$endScriptHeader/scriptArguments"/>"
                    
                    <xsl:if test="$endScriptHeader/runInVirtualEnvironment">
                    , "runInVirtualEnvironment": <xsl:value-of select="$endScriptHeader/runInVirtualEnvironment"/>
                    </xsl:if>
                    
                    <xsl:if test="$endScriptHeader/runOnce">
                    , "runOnce": <xsl:value-of select="$endScriptHeader/runOnce"/>
                    </xsl:if>
                    
                    <xsl:if test="$endScriptHeader/waitForScriptToFinish">
                    , "waitForScriptToFinish": <xsl:value-of select="$endScriptHeader/waitForScriptToFinish"/>
                    </xsl:if>
                    
                    <xsl:if test="$endScriptHeader/timeout">
                    , "timeout": "<xsl:value-of select="$endScriptHeader/timeout"/>"
                    </xsl:if>
                }
            </xsl:if>
            
            
        }
        <xsl:if test="position()!=last()">
        ,
        </xsl:if>
    </xsl:for-each>
    ],
    "processes" : [
    <xsl:for-each select="processes/process">
        {
            "executable": "<xsl:value-of select="executable"/>"
            <xsl:if test="fixups/fixup">
            ,"fixups": [
                <xsl:for-each select="fixups/fixup">
                {
                    <xsl:variable name="dllName" select="dll" />
                    "dll": "<xsl:value-of select="dll"/>"
                    <xsl:if test="contains($dllName, 'WaitForDebuggerFixup')">
                        , "config" :
                        {
                            "enabled": <xsl:value-of select="config/enabled"/>
                        }
                    </xsl:if>
                    <xsl:if test="contains($dllName, 'TraceFixup')">
                        ,"config":
                        {
                            <xsl:if test="config/traceMethod">
                                "traceMethod": "<xsl:value-of select="config/traceMethod"/>"
                            </xsl:if>
                            
                            <xsl:if test="config/traceLevels" >
                                <xsl:if test="config/traceMethod">
                                ,
                                </xsl:if>
                                "traceLevels":
                                {
                                    <xsl:choose>
                                        <xsl:when test="config/traceLevels/traceLevel/@level='default'">
                                            "default": "<xsl:value-of select="config/traceLevels/traceLevel"/>"
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <xsl:for-each select="config/traceLevels/traceLevel">
                                                "<xsl:value-of select="@level"/>": "<xsl:value-of select="current()"/>"
                                                <xsl:if test="position()!=last()">
                                                    ,
                                                </xsl:if>
                                            </xsl:for-each>
                                        </xsl:otherwise>
                                    </xsl:choose>
                                }
                            </xsl:if>
                            
                            <xsl:if test="config/breakOn">
                                , "breakOn":
                                {
                                    <xsl:choose>
                                        <xsl:when test="config/breakOn/break/@level='default'">
                                            "default": "<xsl:value-of select="config/breakOn/break"/>"
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <xsl:for-each select="config/breakOn/break">
                                                "<xsl:value-of select="@level"/>": "<xsl:value-of select="current()"/>"
                                                <xsl:if test="position()!=last()">
                                                    ,
                                                </xsl:if>
                                            </xsl:for-each>
                                        </xsl:otherwise>
                                    </xsl:choose>
                                }
                            </xsl:if>
                            
                            <xsl:if test="config/waitForDebugger">
                            , "waitForDebugger": <xsl:value-of select="config/waitForDebugger"/>
                            </xsl:if>
                            
                            <xsl:if test="config/traceFunctionEntry">
                                , "traceFunctionEntry": <xsl:value-of select="config/traceFunctionEntry"/>
                            </xsl:if>
                            
                            <xsl:if test="config/traceCallingModule">
                                , "traceCallingModule": <xsl:value-of select="config/traceCallingModule"/>
                            </xsl:if>
                            
                            <xsl:if test="config/ignoreDllLoad">
                                ,"ignoreDllLoad": <xsl:value-of select="config/ignoreDllLoad" />
                            </xsl:if>
                        }
                    </xsl:if>
                    <xsl:if test="contains($dllName, 'FileRedirection')">
                        "config":
                        {
                            "redirectedPaths": {
                                <xsl:if test="config/redirectedPaths/packageRelative">
                                    "packageRelative": [
                                    <xsl:for-each select="config/redirectedPaths/packageRelative">
                                        {
                                            "base": "<xsl:value-of select="pathConfig/base"/>",
                                            "patterns": [
                                                <xsl:for-each select="pathConfig/patterns/pattern">
                                                    "<xsl:value-of select="current()"/>"
                                                    <xsl:if test="position()!=last()">
                                                        ,
                                                    </xsl:if>
                                                </xsl:for-each>
                                            ]
                                        }
                                    </xsl:for-each>
                                    ]
                                    <xsl:if test="config/redirectedPaths/packageDriveRelative or config/redirectedPaths/knownFolders">
                                    ,
                                    </xsl:if>
                                </xsl:if>
                                <xsl:if test="config/redirectedPaths/packageDriveRelative">
                                    "packageDriveRelative": [
                                    <xsl:for-each select="config/redirectedPaths/packageDriveRelative">
                                        {
                                            "base": "<xsl:value-of select="pathConfig/base"/>",
                                            "patterns": [
                                                <xsl:for-each select="pathConfig/patterns/pattern">
                                                    "<xsl:value-of select="current()"/>"
                                                    <xsl:if test="position()!=last()">
                                                        ,
                                                    </xsl:if>
                                                </xsl:for-each>
                                            ]
                                        }
                                    </xsl:for-each>
                                    ]
                                </xsl:if>
                                <xsl:if test="config/redirectedPaths/knownFolders">
                                    "knownFolders": [
                                    <xsl:for-each select="config/redirectedPaths/knownFolders/knownFolder">
                                        {
                                            "id": "<xsl:value-of select="id"/>",
                                            "relativePaths": [
                                            <xsl:for-each select="relativePaths/relativePath">
                                            {
                                                "base": "<xsl:value-of select="base"/>",
                                                "patterns": [
                                                    <xsl:for-each select="patterns/pattern">
                                                        "<xsl:value-of select="current()"/>"
                                                        <xsl:if test="position()!=last()">
                                                            ,
                                                        </xsl:if>
                                                    </xsl:for-each>
                                                ]
                                            }
                                            <xsl:if test="position()!=last()">
                                                ,
                                            </xsl:if>
                                            </xsl:for-each>
                                            ]
                                        }
                                        <xsl:if test="position()!=last()">
                                            ,
                                        </xsl:if>
                                    </xsl:for-each>
                                    ]
                                </xsl:if>
                            }
                        }
                    </xsl:if>
                    <xsl:if test="contains($dllName, 'MultiByteToWideCharTestFixup">
                    {
                        "dll": "<xsl:value-of select="dll"/>"
                    }
                    </xsl:if>
                    <xsl:if test="contains($dllName, 'CompositionTestFixup">
                    {
                        "dll": "<xsl:value-of select="dll"/>"
                    }
                    </xsl:if>
                }
                    <xsl:if test="position()!=last()">
                        ,
                    </xsl:if>
                </xsl:for-each>
                ]
            </xsl:if>
        }
        <xsl:if test="position()!=last()">
            ,
        </xsl:if>
    </xsl:for-each>
    ]
}
    </xsl:template>
    <xsl:output omit-xml-declaration="yes"/>
</xsl:stylesheet>