#!/usr/bin/php
<?php
$boxes = array();
$links = array();
$syms = array();
preg_match_all('/Alloc \{\nBacktrace \[((?>0x[0-9a-f]*\=\\\[^\\\\\=]*=\\\, )*)\]\nNum (\d*)\nSz (\d*)\n\}/', file_get_contents($_SERVER['argc']>1?$_SERVER['argv'][1]:"php://stdin"), $allocs, PREG_SET_ORDER);
foreach($allocs as $alloc)
{
    $bt = explode(", ", $alloc[1]);
    $nm = "";
    $sn = 0;
    $o_sn = false;
    for($i=0;$i<8;$i++)
    {
        $sym = explode('=\\', $bt[$i]);
        $nm .= $sym[0];
        $found = false;
        foreach($syms as $k=>$s)
        {
            if($s['sm'] == $nm)
            {
                $found = $k;
                break;
            }
        }

        if($found !== false)
        {
            $syms[$found]['n'] += $alloc[2];
            $syms[$found]['sz'] += $alloc[3];
            $sn = $found;
        }
        else
        {
            $syms[] = array('sm'=>$nm, 'name'=>$sym[1], 'n'=>$alloc[2], 'sz'=>$alloc[3]);
            $sn = count($syms)-1;
        }

        if($o_sn !== false)
            $links[] = "S${sn}->S${o_sn};";
        $o_sn = $sn;
    }
}
foreach($syms as $k=>$sym)
{
    if($sym['sz']>=1024*1024)
        $sz = ($sym['sz']/(1024*1024)).'MB';
    elseif($sym['sz']>=1024)
        $sz = ($sym['sz']/1024).'KB';
    else
        $sz = "$sym[sz]B";
    $boxes[] = "S$k [shape=box, label=\"[$sym[n]/$sz] $sym[name]\"];";
}
$links = array_unique($links);
file_put_contents('a.dot', "digraph some_dia {
compound=true;
size=\"200,200\";
".implode("\n\t", $boxes)."
".implode("\n\t", $links)."
}");
`dot -Tpng -oalloc_tree.png a.dot && rm a.dot`;
?>
