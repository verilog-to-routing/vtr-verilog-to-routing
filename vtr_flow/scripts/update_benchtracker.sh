chmod -R 777 benchtracker
rm -rf benchtracker
git clone https://github.com/LemonPi/benchtracker.git
rm -rf benchtracker/.git
rm -rf benchtracker/.gitignore
chmod -R ugo-w benchtracker
