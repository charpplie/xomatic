Const ForReading = 1, ForWriting = 2, ForAppending = 8

if WScript.Arguments.Count <> 1 then
    WScript.Echo "No output filename specified."
else

    filename = WScript.Arguments(0)

    Dim fs, objNetwork
    Set fs = WScript.CreateObject("Scripting.FileSystemObject")
    Set net = CreateObject("WScript.Network")

    ' Generate the output file contents.
    output = output & "#define BUILD_DATE_STRING """ & FormatDateTime(Now(), vbLongDate) & """" & VBNewLine
    output = output & "#define BUILD_USER_NAME """ & net.UserName & """" & VBNewLine

    ' Check whether the file has changed.
    replaceFile = True
    set f = fs.OpenTextFile(filename, ForReading, True)
    existingText = ""
    if not f.AtEndOfStream then
        existingText = f.ReadAll
    end if
    if existingText = output then
        replaceFile = False
    end if
    f.Close

    ' If the file has changed, then update it. Otherwise don't touch it, or
    ' we will have to rebuild unnecessarily.    
    if replaceFile then
        set f = fs.OpenTextFile(filename, ForWriting, True)
        f.Write(output)
        f.Close
    end if
    
end if
