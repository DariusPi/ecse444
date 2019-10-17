% % Record your voice for 2 seconds.
recObj = audiorecorder;
disp('Start speaking.')
recordblocking(recObj, 2);
disp('End of Recording.');

% % Play back the recording.
play(recObj);

% Store data in double-precision array.
myRecording = getaudiodata(recObj);
figure; plot(myRecording); % Plot the original waveform.

% cut the area you want and convert it into integer
MyAudioArray = uint16((myRecording(1:16000)+1)*2048); 
csvwrite('AudioArray.csv',MyAudioArray');
% Plot the modified waveform.
figure;plot(MyAudioArray);

hexarr=uint8(32000);
for j=1:16000
    hexarr(j*2-1)=idivide(MyAudioArray(j),256);
    hexarr(j*2)=mod(MyAudioArray(j),(256));
end

dest = string(hexarr);
if ~isempty(instrfind)
     fclose(instrfind);
      delete(instrfind);
end
s1=serial('COM3','BaudRate',115200);
fopen(s1);
 
for i=1:32000
    fprintf(s1, '%s', dest(i));
end