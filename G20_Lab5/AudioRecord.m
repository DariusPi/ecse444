% Record your voice for 5 seconds.
recObj = audiorecorder;
disp('Start speaking.')
recordblocking(recObj, 1);
disp('End of Recording.');

% Play back the recording.
play(recObj);

% Store data in double-precision array.
myRecording = getaudiodata(recObj);
figure; plot(myRecording); % Plot the original waveform.

% cut the area you want and convert it into integers
MyAudioArray = uint16((myRecording(1:8000)+1)*2048); 
csvwrite('AudioArray.csv',MyAudioArray');
% Plot the modified waveform.
figure;plot(MyAudioArray);  