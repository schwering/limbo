-- Simple Haskell program that generates KB clauses for the position functions.

n (x,y) = (x,y-1)
e (x,y) = (x+1,y)
s (x,y) = (x,y+1)
w (x,y) = (x-1,y)
ne (x,y) = (x+1,y-1)
se (x,y) = (x+1,y+1)
nw (x,y) = (x-1,y-1)
sw (x,y) = (x-1,y+1)

xrange = [1..4]
yrange = [1..4]

valid (x,y) = x `elem` xrange && y `elem` yrange

kb1 fname (x,y) (x',y') = "KB: "++ fname ++"(p" ++ show x ++ show y ++")=p" ++ show x' ++ show y' ++ " "

kb f fn = unlines $ [ foldr (++) "" $ map (uncurry (kb1 fn)) $ filter (\(_,p) -> valid p) $ map (\p -> (p, f p)) $ [(x,y) | x <- xrange] | y <- yrange]

main = putStrLn $ unlines $ map (uncurry kb) [(n,"n"),(e,"e"),(s,"s"),(w,"w"),(ne,"ne"),(se,"se"),(sw,"sw"),(nw,"nw")]

