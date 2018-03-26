clear all

A = importdata('_photon-counts.txt');
plot(A)

for kk = 1:400
    if mod(kk,2)
        B(:,kk) = A(((kk-1)*400)+(1:400));
    else
        B(:,kk) = flipud( A( ((kk-1)*400)+(1:400) ) );
    end
end

%{
hm = HeatMap(B, 'Colormap','redbluecmap');

close all hidden
ax = hm.plot; % 'ax' will be a handle to a standard MATLAB axes.
colorbar('Peer', ax); % Turn the colorbar on
caxis(ax, [0 10]); % Adjust the color limits
%}