chmod -R 777 benchtracker
mv benchtracker  benchtracker_moved
git clone https://github.com/LemonPi/benchtracker.git
rm -rf benchtracker/.git
rm -rf benchtracker/.gitignore
if [ -d "benchtracker_moved/.svn" ]; then
    mv benchtracker_moved/.svn benchtracker
fi
sed -i "s/app.run(debug=True, port=port)/app.run(host='0.0.0.0', port=port)/" benchtracker/server_db.py
rm -rf bentracker_moved
