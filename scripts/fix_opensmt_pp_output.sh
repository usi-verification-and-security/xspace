cp $1 ${1}.bak

sed -ri 's|-([0-9]+[0-9/]*)|(- \1)|g' $1 && sed -ri 's|([0-9]+)/([0-9]+)|(/ \1 \2)|g' $1
