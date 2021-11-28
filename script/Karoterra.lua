local K = {}

function K.ary2str(ary)
  local s = ""
  for i = 1, #ary do
    s = s .. tostring(ary[i]) .. " "
  end
  return s
end

function K.line(obj, x1, y1, x2, y2, w, color, alpha)
  w = w or 1
  color = color or 0xffffff
  alpha = alpha or 1

  obj.load("figure", "éläpå`", color, 1)
  rad = math.atan2(y2-y1, x2-x1) + math.pi / 2
  local c, s = w * math.cos(rad), w * math.sin(rad)
  obj.drawpoly(
    x1 + c, y1 + s, 0,
    x1 - c, y1 - s, 0,
    x2 - c, y2 - s, 0,
    x2 + c, y2 + s, 0,
    0,0, 1,0, 1,1, 0,1, alpha
  )
end

function K.polyline(obj, xy, ox, oy, w, color, alpha)
  for i = 1, #xy, 2 do
    local i2 = (i+1) % #xy + 1
    K.line(obj, xy[i]+ox, xy[i+1]+oy, xy[i2]+ox, xy[i2+1]+oy, w, color, alpha)
  end
end

return K
