sudo iwpan phy phy0 set channel 0 23
sudo iwpan dev wpan0 set pan_id 0x23
sudo ip link add link wpan0 name lowpan0 type lowpan
sudo ip link set wpan0 up
sudo ip link set lowpan0 up

systemctl start radvd
sudo ip addr add 2001:67c:254:b0b2:affe:3000::1/84 dev lowpan0
