<?php
  include('dbms_inc.php');

  $link = mysql_connect($mysql_host,$mysql_user,$mysql_password)
	or die("Could not connect");

  mysql_select_db($mysql_dbms) or die("Could not select database");

  $sorttype = $_GET['sorttype'];

  if ("$sorttype" == "" || $sorttype < 1 || $sorttype > 4)
  {
    $sorttype = 1;
  }

  $join_on = "";
  
  switch ($sorttype)
  {
  case 1: // OS
    $order_by = "osname,osversion,vendorid,description,mtxversion";
    break;
  case 2: // Vendor
    $order_by = "vendorid,description,osname,osversion,mtxversion";
    break;
  case 3: // Description
    $order_by = "description,vendorid,osname,osversion,mtxversion";
    break;
  case 4: // MTX Version
    $order_by = "`key`,osname,osversion,vendorid";
    $join_on = "inner join versions on mtxversion = version";
    break;
  }

  $query_str = "select id,osname,osversion,vendorid,description,mtxversion from loaders $join_on where enabled = 1 order by $order_by";
  $result = mysql_query($query_str,$link) or die("Invalid query '$query_str'");
  $num_rows = mysql_num_rows($result);
  $lines_per_page = $_GET['count'];

  if ("$lines_per_page" == "")
  {
    $lines_per_page = 25;
  }

  switch ($lines_per_page)
  {
  case 0:
  case 10:
  case 25:
  case 50:
  case 75:
  case 100:
    break;
  default:
    $lines_per_page = 25;
    break;
  }
  
  if ($lines_per_page > 0)
  {
    $num_pages = ceil($num_rows / $lines_per_page);
  }
  else
  {
    $num_pages = 1;
  }

  $page_number = $_GET['start'];

  if ("$page_number" == "" || $page_number == 0)
  {
    $page_number = 1;
  }

  if ($page_number > $num_pages)
  {
    $page_number = $num_pages;
  }
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta http-equiv="Content-type" content="text/html;charset=UTF-8" />
    <meta http-equiv="Pragma" content="no-cache" />

    <title>MTX Compatibility List - Summary</title>

    <style type="text/css">
      <!--
        TH { background-color: aqua; }
        TR { background-color: white; }
        TD { text-align: left; }
        H2 { text-align: center; }
        BODY { background-color: white; }
        A:link { color: blue; text-decoration: underline; }
        A:active { text-decoration: underline; }
        A:hover { text-decoration: underline; }
        A:visited { color: blue; text-decoration: underline; }
      -->
    </style>

    <script type="text/javascript">
      <!--
        function validateNumeric(e)
        {
          var keyCode;

          if (e.which)
          {
             keyCode = e.which;
          }
          else if (e.keyCode)
          {
             keyCode = e.keyCode;
          }
          else 
          {
             return true;
          }

          return keyCode < 32 || keyCode == null || (keyCode >= 48 && keyCode <= 57);
        }

        function goto_page()
        {
          var page_number = document.getElementById('page_number').value;
          var lines_per_page = document.getElementById('lines_per_page').value;
          var sort_type = <?php echo $sorttype; ?>;

          location.href = "compatibility.php?sorttype="+sort_type+"&start="+page_number+"&count="+lines_per_page;
        }

        function change_lines_per_page()
        {
          var lines_per_page;
          var first_line;
          var page_number;
          var sort_type;

          lines_per_page = document.getElementById('lines_per_page').value;
          if (lines_per_page > 0)
          {
            first_line = <?php echo $lines_per_page * ($page_number - 1) + 1; ?>;
            page_number = parseInt(first_line / lines_per_page) + 1;
            sort_type = <?php echo $sorttype; ?>;
          }
          else
          {
            first_line = 1;
            page_number = 1;
          }
          sort_type = <?php echo $sorttype; ?>;

          location.href = "compatibility.php?sorttype="+sort_type+"&start="+page_number+"&count="+lines_per_page;
        }

        function goto_prev()
        {
          var page_number = document.getElementById('page_number').value;
          var lines_per_page = document.getElementById('lines_per_page').value;
          var sort_type = <?php echo $sorttype; ?>;

          if (page_number <= 2)
          {
            page_number = 1;
          }
          else
          {
            page_number -= 1;
          }
          
          location.href = "compatibility.php?sorttype="+sort_type+"&start="+page_number+"&count="+lines_per_page;
        }

        function goto_next()
        {
          var page_number = document.getElementById('page_number').value;
          var lines_per_page = document.getElementById('lines_per_page').value;
          var sort_type = <?php echo $sorttype; ?>;

          page_number++;

          location.href = "compatibility.php?sorttype="+sort_type+"&start="+page_number+"&count="+lines_per_page;
        }
      -->
	</script>
  </head>

  <body>
    <table style="width:100%" cellspacing="0" cellpadding="10">
      <tr valign="middle">
        <th></th>
        <th style="text-align: center">
          <h1>MTX Compatibility List<br />Summary</h1>
        </th>
      </tr>
      <tr>
        <th style="vertical-align:top; text-align:left">
          <h2>Links:</h2>
          <p>
            <a href="http://sourceforge.net/projects/mtx">SourceForge Project</a>
          </p>
          <p>
            <a href="http://mtx.opensource-sw.net">Home Page</a>
          </p>
          <p>
            <a href="http://sourceforge.net/project/showfiles.php?group_id=4626">Downloads</a>
          </p>
          <p style="font-weight:bold; text-decoration:underline">Compatibility</p>
          <p>
            <span style="margin-left: 1em; font-weight:bold">View</span>
            <br />
            <a href="contrib.php" style="margin-left: 1em">Submit</a>
            <br />
          </p>
          <p>
            <a href="faq.html">FAQ</a>
          </p>
        </th>
        <td rowspan="2">
          The database is currently very incomplete. Please <a href="contrib.php">submit</a> new 
          entries so that others can benefit.
          <p />
          You can change the sort order by clicking on the underlined column heading.
          <p />
          In order to display the detailed information click on the desired line.
          <p />
          <table border="1" style="width:100%">
            <tr>
              <td colspan="<?php echo $sorttype == 4 ? "3" : "2" ?>" style="text-align:left; border-style:none">
                Lines per page:
                <select id="lines_per_page" onchange="change_lines_per_page()">
                  <option value="10" <?php if ($lines_per_page == 10) echo 'selected="selected"'?>>10</option>
                  <option value="25" <?php if ($lines_per_page == 25) echo 'selected="selected"'?>>25</option>
                  <option value="50" <?php if ($lines_per_page == 50) echo 'selected="selected"'?>>50</option>
                  <option value="75" <?php if ($lines_per_page == 75) echo 'selected="selected"'?>>75</option>
                  <option value="100" <?php if ($lines_per_page == 100) echo 'selected="selected"'?>>100</option>
                  <option value="0" <?php if ($lines_per_page == 0) echo 'selected="selected"'?>>All</option>
                </select>
              </td>
              <td colspan="<?php echo $sorttype == 4? "2" : "3" ?>" style="text-align:right; border-style:none">
                <input type="button" value="Previous" style="width:6em" onclick="goto_prev()" 
                  <?php if ($page_number < 2) echo 'disabled="disabled"'; ?>
                />
                Page 
                <input type="text" id="page_number" style="width:2em" maxlength="4" 
                  <?php echo 'value="'.$page_number.'"' ?> 
                  onkeypress="return validateNumeric(event)" onchange="goto_page()" />
                of
                <?php echo "$num_pages\n" ?>
                <input type="button" value="Next" style="width:6em" onclick="goto_next()" 
                  <?php if ($page_number >= $num_pages) echo "disabled=\"disabled\"\n"; ?>
                />
              </td>
            </tr>
            <tr>
<?php 
  if ($sorttype == 1) {
?>
              <th>OS</th>
              <th>OS Version</th>
              <th>
                <a href="compatibility.php?sorttype=2">Vendor</a>
              </th>
              <th>
                <a href="compatibility.php?sorttype=3">Description</a>
              </th>
              <th>
                <a href="compatibility.php?sorttype=4">MTX Version</a>
              </th>
<?php 
  } else if ($sorttype == 2) {
?>
              <th>Vendor</th>
              <th>
                <a href="compatibility.php?sorttype=3">Description</a>
              </th>
              <th>
                <a href="compatibility.php?sorttype=1">OS</a>
              </th>
              <th>OS Version</th>
              <th>
                <a href="compatibility.php?sorttype=4">MTX Version</a>
              </th>
<?php 
  } else if ($sorttype == 3) {
?>
              <th>Description</th>
              <th>
                <a href="compatibility.php?sorttype=2">Vendor</a>
              </th>
              <th>
                <a href="compatibility.php?sorttype=1">OS</a>
              </th>
              <th>OS Version</th>
              <th>
                <a href="compatibility.php?sorttype=4">MTX Version</a>
              </th>
<?php 
  } else {
?>
              <th>MTX Version</th>
              <th>
                <a href="compatibility.php?sorttype=1">OS</a>
              </th>
              <th>OS Version</th>
              <th>
                <a href="compatibility.php?sorttype=2">Vendor</a>
              </th>
              <th>
                <a href="compatibility.php?sorttype=3">Description</a>
              </th>
<?php
  }
?>
            </tr>
<?php
  if ($num_rows==0) {
?>
            <tr>
              <th style="background-color: white">NO RECORDS IN DATABASE</th>
            </tr>
<?php 
  }
  else
  {
    if ($lines_per_page > 0)
    {
      $first_row = ($page_number - 1) * $lines_per_page;
    }
    else
    {
      $first_row = 0;
    }

    if ($first_row > 0)
    {
      mysql_data_seek($result, $first_row);
    }

    for ($index = 0; $lines_per_page == 0 || $index < $lines_per_page; $index += 1)
    {
      
      $row = mysql_fetch_assoc($result);

      if (! $row)
      {
        break;
      }
?>
            <tr onclick="location.href=&quot;detail.php?record=<?php echo $row['id'] ?>&quot;">

<?php 
      if ($sorttype == 1) {
?>
              <td><?php echo $row['osname'] ?><br/></td>
              <td><?php echo $row['osversion'] ?><br/></td>
              <td><?php echo $row['vendorid'] ?><br/></td>
              <td><?php echo $row['description'] ?><br/></td>
              <td><?php echo $row['mtxversion'] ?><br/></td>
<?php 
      } else if ($sorttype == 2) {
?>
              <td><?php echo $row['vendorid'] ?><br/></td>
              <td><?php echo $row['description'] ?><br/></td>
              <td><?php echo $row['osname'] ?><br/></td>
              <td><?php echo $row['osversion'] ?><br/></td>
              <td><?php echo $row['mtxversion'] ?><br/></td>
<?php 
      } else if ($sorttype == 3) {
?>
              <td><?php echo $row['description'] ?><br/></td>
              <td><?php echo $row['vendorid'] ?><br/></td>
              <td><?php echo $row['osname'] ?><br/></td>
              <td><?php echo $row['osversion'] ?><br/></td>
              <td><?php echo $row['mtxversion'] ?><br/></td>
<?php 
      } else {
?>
              <td><?php echo $row['mtxversion'] ?><br/></td>
              <td><?php echo $row['osname'] ?><br/></td>
              <td><?php echo $row['osversion'] ?><br/></td>
              <td><?php echo $row['vendorid'] ?><br/></td>
              <td><?php echo $row['description'] ?><br/></td>
<?php
      }
      echo "            </tr>\n";
    }
    mysql_free_result($result);
  }
?>
		  </table>
		  <br />
        </td>
      </tr>
      <tr>
        <th style="text-align:center; vertical-align:middle">
          <p>
            <a href="http://www.dreamhost.com/r.cgi?277748">
              <img src="dh-100x75.gif" alt="DreamHost.com Logo"
                   height="75" width="100" style="border:0" />
            </a>
          </p>
          <p>
            <a href="http://validator.w3.org/check?uri=referer" >
              <img src="valid-xhtml10.png" alt="Valid XHTML 1.0 Strict" 
                   height="31" width="88" style="border:0" />
            </a>
          </p>
          <p>
            <a href="http://sourceforge.net/projects/mtx">
              <img src="http://sflogo.sourceforge.net/sflogo.php?group_id=4626&amp;type=16"
                   width="150" height="40"
                   alt="Get MTX: Media Changer Tools at SourceForge.net. Fast, secure and Free Open Source software downloads" />
            </a>
          </p>
        </th>
      </tr>
	  <tr>
		<th />
		<td>
          <hr />
          <table style="font-size:small; width:100%">
            <tr>
              <td style="text-align:left; width:33%">
                Maintained by <a href="mailto:robertnelson@users.sourceforge.net">Robert Nelson</a>
              </td>
              <td style="text-align:center; width:34%">
<?php
				$ChangedDate = preg_replace('/.*: (.+) \(.*/', '\1', '$LastChangedDate: 2009-04-25 05:56:37 +0000 (Sat, 25 Apr 2009) $');
				echo "Date changed: $ChangedDate";
?>
              </td>
              <td style="text-align:right; width:33%">
<?php
				$ChangedBy = preg_replace('/.*: (.+) \$/', '\1', '$LastChangedBy: robertnelson $');
				echo "Changed by: $ChangedBy";
?>
              </td>
            </tr>
          </table>
		</td>
	  </tr>
    </table>
  </body>
</html>
