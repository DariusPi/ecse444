% % Record your voice for 1 seconds.
% recObj = audiorecorder;
% disp('Start speaking.')
% recordblocking(recObj, 1);
% disp('End of Recording.');
% 
% % Play back the recording.
% play(recObj);
% 
% % Store data in double-precision array.
% myRecording = getaudiodata(recObj);
% figure; plot(myRecording); % Plot the original waveform.
% 
% % cut the area you want and convert it into integer
% MyAudioArray = uint16((myRecording(1:8000)+1)*1024/2); 
% csvwrite('AudioArray.csv',MyAudioArray');

% % Plot the modified waveform.
% figure;plot(MyAudioArray);

% hexarr=uint8(32000);
% for j=1:8000
%     hexarr((j*4))=mod(data(j),(16));
%     hexarr ((j*4)-1)=mod((idivide(MyAudioArray(j),16)),16);
%     hexarr ((j*4)-2)=mod((idivide(MyAudioArray(j),256)),16);
%     hexarr ((j*4)-3)=mod((idivide(MyAudioArray(j),4096)),16);
%     
% end
% 
% dest = string(dec2hex(hexarr));

if ~isempty(instrfind)
     fclose(instrfind);
      delete(instrfind);
end
s1=serial('COM3','BaudRate',115200);
fopen(s1);
 
for i=1:32000
    fprintf(s1, '%s',(dest(i)));
end
